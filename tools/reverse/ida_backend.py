from __future__ import annotations

from pathlib import Path
from typing import Any
import warnings

warnings.filterwarnings("ignore", category=DeprecationWarning)
warnings.filterwarnings("ignore", category=ResourceWarning)

import idapro

import ida_funcs
import ida_hexrays
import ida_ida
import ida_idaapi
import ida_segment
import idautils


class IdaDatabase:
    def __init__(self, path: Path):
        self.path = Path(path)
        self._opened = False

    def __enter__(self) -> "IdaDatabase":
        idapro.enable_console_messages(False)
        result = idapro.open_database(str(self.path), True)
        if result != 0:
            raise RuntimeError(f"failed to open IDA database: {self.path} ({result})")
        self._opened = True
        return self

    def __exit__(self, exc_type: Any, exc: Any, traceback: Any) -> None:
        if self._opened:
            idapro.close_database(False)
            self._opened = False

    def processor_name(self) -> str:
        self._require_open()
        return ida_ida.inf_get_procname()

    def segment_count(self) -> int:
        self._require_open()
        return ida_segment.get_segm_qty()

    def function_count(self) -> int:
        self._require_open()
        return ida_funcs.get_func_qty()

    def find_exact_string(self, value: str) -> int:
        self._require_open()
        for string in idautils.Strings():
            if str(string) == value:
                return int(string.ea)
        raise KeyError(f"string not found: {value}")

    def xrefs_to(self, address: int) -> list[int]:
        self._require_open()
        return sorted({int(ref.frm) for ref in idautils.XrefsTo(address)})

    def containing_function(self, address: int) -> dict[str, str]:
        self._require_open()
        start = ida_funcs.get_func_start(address)
        if start == ida_idaapi.BADADDR:
            raise KeyError(f"function not found for address: {address:#x}")
        end = ida_funcs.get_next_func_ea(start)
        if end == ida_idaapi.BADADDR:
            segment = ida_segment.getseg(start)
            end = segment.end_ea if segment is not None else start
        return {
            "start": f"{start:#x}",
            "end": f"{end:#x}",
            "name": ida_funcs.get_func_name(start),
        }

    def decompile_summary(self, address: int, max_lines: int = 12) -> list[str]:
        self._require_open()
        if not ida_hexrays.init_hexrays_plugin():
            return []
        try:
            decompiled = ida_hexrays.decompile(address)
        except Exception:
            return []
        if decompiled is None:
            return []
        pseudocode = str(decompiled)
        lines = [line.strip() for line in pseudocode.splitlines() if line.strip()]
        return lines[:max_lines]

    def _require_open(self) -> None:
        if not self._opened:
            raise RuntimeError("IDA database is not open")
