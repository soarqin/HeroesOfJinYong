"""Linear-Executable analysis backend for the original battle binaries.

`Z.DAT` is not 16-bit DOS code: its MZ part is only a DOS/4GW style stub and the
real program is a 32-bit LE (Linear Executable) image appended to it. This
module reproduces the loader's flat address space, applies the internal fixup
records, and disassembles it with capstone, so battle evidence can be
regenerated on machines without IDA.

Addresses produced here are the loaded virtual addresses, which are the same
values the IDA-based backend reports.
"""

from __future__ import annotations

import bisect
import struct
from dataclasses import dataclass
from pathlib import Path

STACK_CHECK_HINT = 0x3ED1E
"""Watcom stack-probe helper; every non-leaf function starts by calling it."""


@dataclass(frozen=True)
class LEObject:
    index: int
    base: int
    virtual_size: int
    flags: int
    first_page: int
    page_count: int


class LEImage:
    """Flat, fixed-up load image of an LE executable."""

    PAGE_HEADER_SIZE = 4
    OBJECT_ENTRY_SIZE = 24

    def __init__(self, path: Path):
        self.path = Path(path)
        raw = self.path.read_bytes()
        if raw[:2] not in (b"MZ", b"ZM"):
            raise ValueError(f"not an MZ file: {self.path}")
        header_offset = struct.unpack_from("<I", raw, 0x3C)[0]
        if raw[header_offset:header_offset + 2] != b"LE":
            raise ValueError(f"no LE header at {header_offset:#x}: {self.path}")
        self.raw = raw
        self._header = header_offset
        base = header_offset
        self.page_count = struct.unpack_from("<I", raw, base + 0x14)[0]
        self.entry_object, self.entry_offset = struct.unpack_from("<II", raw, base + 0x18)
        self.page_size = struct.unpack_from("<I", raw, base + 0x28)[0]
        object_table, object_count = struct.unpack_from("<II", raw, base + 0x40)
        page_table = struct.unpack_from("<I", raw, base + 0x48)[0]
        self.fixup_page_table, self.fixup_record_table = struct.unpack_from("<II", raw, base + 0x68)
        self.data_pages = struct.unpack_from("<I", raw, base + 0x80)[0]
        self._page_table = base + page_table

        self.objects: list[LEObject] = []
        for index in range(object_count):
            offset = base + object_table + index * self.OBJECT_ENTRY_SIZE
            size, obj_base, flags, first, count, _ = struct.unpack_from("<IIIIII", raw, offset)
            self.objects.append(LEObject(index + 1, obj_base, size, flags, first, count))

        top = max(item.base + item.virtual_size for item in self.objects)
        self.image = bytearray(top + self.page_size)
        self._page_base: dict[int, int] = {}
        self.mapped: list[tuple[int, int]] = []
        for item in self.objects:
            for step in range(item.page_count):
                page = item.first_page + step
                file_offset = self.data_pages + (page - 1) * self.page_size
                address = item.base + step * self.page_size
                chunk = raw[file_offset:file_offset + self.page_size]
                self.image[address:address + len(chunk)] = chunk
                self._page_base[page] = address
            self.mapped.append((item.base, item.base + item.page_count * self.page_size))
        self.fixup_count = self._apply_fixups()

    @property
    def entry(self) -> int:
        return self.objects[self.entry_object - 1].base + self.entry_offset

    @property
    def code_range(self) -> tuple[int, int]:
        """Loaded range of the largest object, which holds the program code."""
        largest = max(self.objects, key=lambda item: item.page_count)
        return largest.base, largest.base + largest.page_count * self.page_size

    def read(self, address: int, size: int) -> bytes:
        return bytes(self.image[address:address + size])

    def find(self, pattern: bytes, start: int = 0) -> int:
        return self.image.find(pattern, start)

    def find_all(self, pattern: bytes) -> list[int]:
        found = []
        at = self.image.find(pattern)
        while at >= 0:
            found.append(at)
            at = self.image.find(pattern, at + 1)
        return found

    def find_string(self, value: str) -> int:
        """Address of a NUL-terminated exact string, or -1."""
        needle = value.encode("latin1") + b"\x00"
        at = self.image.find(needle)
        return at if at < 0 else at

    def _apply_fixups(self) -> int:
        raw, base = self.raw, self._header
        table = base + self.fixup_record_table
        page_offsets = [
            struct.unpack_from("<I", raw, base + self.fixup_page_table + 4 * index)[0]
            for index in range(self.page_count + 1)
        ]
        applied = 0
        for page in range(1, self.page_count + 1):
            cursor = table + page_offsets[page - 1]
            end = table + page_offsets[page]
            page_base = self._page_base.get(page)
            while cursor < end:
                source, flags = raw[cursor], raw[cursor + 1]
                cursor += 2
                source_type = source & 0x0F
                source_list = bool(source & 0x20)
                if source_list:
                    count = raw[cursor]
                    cursor += 1
                    sources: list[int] = []
                else:
                    count = 1
                    sources = [struct.unpack_from("<h", raw, cursor)[0]]
                    cursor += 2
                target_kind = flags & 0x03
                object_number = None
                target_offset = 0
                if target_kind == 0:
                    if flags & 0x40:
                        object_number = struct.unpack_from("<H", raw, cursor)[0]
                        cursor += 2
                    else:
                        object_number = raw[cursor]
                        cursor += 1
                    if source_type != 2:
                        if flags & 0x10:
                            target_offset = struct.unpack_from("<I", raw, cursor)[0]
                            cursor += 4
                        else:
                            target_offset = struct.unpack_from("<H", raw, cursor)[0]
                            cursor += 2
                else:
                    cursor += 2 if flags & 0x40 else 1
                    cursor += 4 if flags & 0x10 else 2
                if source_list:
                    for _ in range(count):
                        sources.append(struct.unpack_from("<h", raw, cursor)[0])
                        cursor += 2
                if flags & 0x04:
                    cursor += 4 if flags & 0x20 else 2
                if target_kind != 0 or object_number is None or page_base is None:
                    continue
                if not 1 <= object_number <= len(self.objects):
                    continue
                target = self.objects[object_number - 1].base + target_offset
                for source_offset in sources:
                    address = page_base + source_offset
                    if address < 0 or address + 4 > len(self.image):
                        continue
                    if source_type == 7:
                        struct.pack_into("<I", self.image, address, target & 0xFFFFFFFF)
                    elif source_type == 5:
                        struct.pack_into("<H", self.image, address, target & 0xFFFF)
                    elif source_type == 8:
                        struct.pack_into("<i", self.image, address, target - (address + 4))
                    else:
                        continue
                    applied += 1
        return applied


class FunctionIndex:
    """Function boundaries and cross references for an `LEImage`."""

    def __init__(self, image: LEImage, stack_check: int = STACK_CHECK_HINT):
        try:
            import capstone
        except ImportError as error:  # pragma: no cover - environment dependent
            raise RuntimeError("capstone is required for FunctionIndex") from error
        self._capstone = capstone
        self._md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
        self._md.detail = True
        self.image = image
        self.low, self.high = image.code_range
        self.stack_check = stack_check
        self.prologues = self._find_prologues()
        self.call_sites = self._find_call_sites()
        self.functions = self._merge_starts()
        self._body: dict[int, list] = {}

    def _in_code(self, address: int) -> bool:
        return self.low <= address < self.high

    def _find_prologues(self) -> list[int]:
        """Starts identified by the `push <frame>; call <stack check>` pattern."""
        image = self.image.image
        found = []
        for address in range(self.low, self.high - 10):
            opcode = image[address]
            if opcode == 0x6A:
                call = address + 2
            elif opcode == 0x68:
                call = address + 5
            else:
                continue
            if image[call] != 0xE8:
                continue
            relative = struct.unpack_from("<i", image, call + 1)[0]
            if call + 5 + relative == self.stack_check:
                found.append(address)
        return found

    def _find_call_sites(self) -> dict[int, list[int]]:
        """Every `call rel32` target in the code range, mapped to its sites."""
        image = self.image.image
        sites: dict[int, list[int]] = {}
        for address in range(self.low, self.high - 5):
            if image[address] != 0xE8:
                continue
            relative = struct.unpack_from("<i", image, address + 1)[0]
            target = address + 5 + relative
            if self._in_code(target):
                sites.setdefault(target, []).append(address)
        return sites

    def _merge_starts(self) -> list[int]:
        """Prologue starts plus call targets that land on an instruction start."""
        known = set(self.prologues)
        boundaries = sorted(known)
        aligned = set()
        for index, start in enumerate(boundaries):
            end = boundaries[index + 1] if index + 1 < len(boundaries) else self.high
            for instruction in self._md.disasm(self.image.read(start, end - start), start):
                aligned.add(instruction.address)
        extra = {target for target in self.call_sites
                 if target not in known and target in aligned}
        return sorted(known | extra)

    def function_end(self, start: int) -> int:
        index = bisect.bisect_right(self.prologues, start)
        return self.prologues[index] if index < len(self.prologues) else self.high

    def body(self, start: int) -> list:
        """Linear disassembly from `start` to the next prologue-based start."""
        if start not in self._body:
            end = self.function_end(start)
            self._body[start] = list(self._md.disasm(self.image.read(start, end - start), start))
        return self._body[start]

    def owner(self, address: int) -> int | None:
        index = bisect.bisect_right(self.prologues, address)
        return self.prologues[index - 1] if index else None

    def callers(self, target: int) -> list[int]:
        return list(self.call_sites.get(target, ()))

    def immediates(self, start: int) -> set[int]:
        values = set()
        for instruction in self.body(start):
            for operand in instruction.operands:
                if operand.type == self._capstone.x86.X86_OP_IMM:
                    values.add(operand.imm)
        return values

    def find_by_immediates(self, required: set[int]) -> list[int]:
        return [start for start in self.functions if required <= self.immediates(start)]

    def listing(self, start: int, low: int | None = None, high: int | None = None) -> list[str]:
        lines = []
        for instruction in self.body(start):
            if low is not None and instruction.address < low:
                continue
            if high is not None and instruction.address > high:
                continue
            lines.append(f"{instruction.address:06X}  {instruction.mnemonic:<7} {instruction.op_str}")
        return lines
