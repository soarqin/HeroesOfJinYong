#include "mapwithevent.hh"

#include "window.hh"
#include "menu.hh"
#include "content/constants.hh"
#include "content/event.hh"
#include "world/bag.hh"
#include "world/savedata.hh"
#include "util/conv.hh"
#include "util/math.hh"
#include "util/random.hh"

#include <fmt/format.h>
#include <fmt/printf.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace hojy::scene {
namespace {

enum {
    OrigWidth = 320,
    OrigHeight = 200,
};

event::VmResult completed() {
    return {event::VmStatus::Completed, 1, {}};
}

event::VmResult waiting() {
    return {event::VmStatus::Waiting, 1, {}};
}

event::VmResult fault(const char *message) {
    return {event::VmStatus::Faulted, 0, message};
}

bool resolve(event::EventMemory &memory, std::int16_t flags, std::int16_t operand,
             std::int16_t bit, std::int16_t &value) {
    if ((flags & bit) == 0) {
        value = operand;
        return true;
    }
    return memory.readWord(operand, value);
}

bool addAddress(std::int16_t base, std::int16_t offset, std::int32_t &address) {
    address = static_cast<std::int32_t>(base) + static_cast<std::int32_t>(offset);
    return address >= event::EventMemory::MinWordAddress
        && address <= event::EventMemory::MaxWordAddress;
}

template <typename T>
bool readFieldWord(const T &object, std::int16_t offset, std::int16_t &value) {
    if (offset < 0 || (offset & 1) != 0
        || static_cast<std::size_t>(offset) + sizeof(value) > sizeof(T)) {
        return false;
    }
    const auto *bytes = static_cast<const char *>(static_cast<const void *>(&object));
    std::memcpy(&value, bytes + offset, sizeof(value));
    return true;
}

template <typename T>
bool writeFieldWord(T &object, std::int16_t offset, std::int16_t value) {
    T candidate = object;
    if (offset < 0 || (offset & 1) != 0
        || static_cast<std::size_t>(offset) + sizeof(value) > sizeof(T)) {
        return false;
    }
    auto *bytes = static_cast<char *>(static_cast<void *>(&candidate));
    std::memcpy(bytes + offset, &value, sizeof(value));
    object = candidate;
    return true;
}

bool readSubMapEventWord(const ::hojy::world::state::SubMapEvent &eventData, std::int16_t index,
                         std::int16_t &value) {
    switch (index) {
    case 0: value = eventData.blocked; return true;
    case 1: value = eventData.index; return true;
    case 2: value = eventData.event[0]; return true;
    case 3: value = eventData.event[1]; return true;
    case 4: value = eventData.event[2]; return true;
    case 5: value = eventData.currTex; return true;
    case 6: value = eventData.endTex; return true;
    case 7: value = eventData.begTex; return true;
    case 8: value = eventData.texDelay; return true;
    case 9: value = eventData.x; return true;
    case 10: value = eventData.y; return true;
    default: return false;
    }
}

bool writeSubMapEventWord(::hojy::world::state::SubMapEvent &eventData, std::int16_t index,
                          std::int16_t value) {
    switch (index) {
    case 0: eventData.blocked = value; return true;
    case 1: eventData.index = value; return true;
    case 2: eventData.event[0] = value; return true;
    case 3: eventData.event[1] = value; return true;
    case 4: eventData.event[2] = value; return true;
    case 5: eventData.currTex = value; return true;
    case 6: eventData.endTex = value; return true;
    case 7: eventData.begTex = value; return true;
    case 8: eventData.texDelay = value; return true;
    case 9: eventData.x = value; return true;
    case 10: eventData.y = value; return true;
    default: return false;
    }
}

inline std::pair<int, int> transformOffset(std::int16_t &x, std::int16_t &y,
                                            int ww, int wh) {
    int w = ww, h = ww * OrigHeight / OrigWidth;
    if (h > wh) {
        h = wh;
        w = wh * OrigWidth / OrigHeight;
    }
    x = (ww - w) / 2 + w * x / OrigWidth;
    y = (wh - h) / 2 + h * y / OrigHeight;
    return util::calcSmallestDivision(w, OrigWidth);
}

}

void MapWithEvent::ensureExtendedNode() {
    if (!extendedNode_) {
        extendedNode_ = new ExtendedNode(this, 0, 0, width_, height_);
    }
}

event::VmResult MapWithEvent::execute(const event::Instruction &instruction,
                                      event::EventMemory &memory) {
    const auto v0 = instruction.opcode;
    const auto v1 = instruction.operands[0];
    const auto v2 = instruction.operands[1];
    const auto v3 = instruction.operands[2];
    const auto v4 = instruction.operands[3];
    const auto v5 = instruction.operands[4];
    const auto v6 = instruction.operands[5];

    auto mov = [&](std::int16_t operand, std::int16_t bits,
                   std::int16_t &value) {
        return resolve(memory, v1, operand, bits, value);
    };
    auto write = [&](std::int32_t address, std::int16_t value) {
        return memory.writeWord(address, value);
    };
    auto read = [&](std::int32_t address, std::int16_t &value) {
        return memory.readWord(address, value);
    };

    switch (v0) {
    case 8: {
        std::int16_t value = 0;
        if (!mov(v2, 1, value)) { return fault("event memory read out of range"); }
        const auto &text = ::hojy::content::gEvent.origTalk(value);
        if (!memory.writeCString(v3, text)) {
            return fault("event talk string exceeds memory");
        }
        return completed();
    }
    case 9: {
        std::int16_t value = 0;
        if (!mov(v4, 1, value)) { return fault("event format operand out of range"); }
        const auto format = memory.readCString(v3);
        if (!format) { return fault("event format string is not terminated"); }
        try {
            const auto rendered = fmt::sprintf(*format, value);
            if (!memory.writeCString(v2, rendered)) {
                return fault("formatted event string exceeds memory");
            }
        } catch (...) {
            return fault("invalid event format string");
        }
        return completed();
    }
    case 16: {
        std::int16_t index = 0, offset = 0, value = 0;
        if (!mov(v3, 1, index) || !mov(v4, 2, offset) || !mov(v5, 4, value)) {
            return fault("event field operand out of range");
        }
        bool ok = false;
        switch (v2) {
        case 0: if (auto *p = ::hojy::world::state::gSaveData.charInfo[index]) { ok = writeFieldWord(*p, offset, value); } break;
        case 1: if (auto *p = ::hojy::world::state::gSaveData.itemInfo[index]) { ok = writeFieldWord(*p, offset, value); } break;
        case 2: if (auto *p = ::hojy::world::state::gSaveData.subMapInfo[index]) { ok = writeFieldWord(*p, offset, value); } break;
        case 3: if (auto *p = ::hojy::world::state::gSaveData.skillInfo[index]) { ok = writeFieldWord(*p, offset, value); } break;
        case 4: if (auto *p = ::hojy::world::state::gSaveData.shopInfo[index]) { ok = writeFieldWord(*p, offset, value); } break;
        default: return fault("unknown event field table");
        }
        return ok ? completed() : fault("event field write out of range");
    }
    case 17: {
        std::int16_t index = 0, offset = 0, destination = v5, value = 0;
        if (!mov(v3, 1, index) || !mov(v4, 2, offset)) {
            return fault("event field operand out of range");
        }
        bool ok = false;
        switch (v2) {
        case 0: if (auto *p = ::hojy::world::state::gSaveData.charInfo[index]) { ok = readFieldWord(*p, offset, value); } break;
        case 1: if (auto *p = ::hojy::world::state::gSaveData.itemInfo[index]) { ok = readFieldWord(*p, offset, value); } break;
        case 2: if (auto *p = ::hojy::world::state::gSaveData.subMapInfo[index]) { ok = readFieldWord(*p, offset, value); } break;
        case 3: if (auto *p = ::hojy::world::state::gSaveData.skillInfo[index]) { ok = readFieldWord(*p, offset, value); } break;
        case 4: if (auto *p = ::hojy::world::state::gSaveData.shopInfo[index]) { ok = readFieldWord(*p, offset, value); } break;
        default: return fault("unknown event field table");
        }
        if (!ok || !write(destination, value)) {
            return fault("event field read out of range");
        }
        return completed();
    }
    case 18: {
        std::int16_t index = 0, value = 0;
        if (!mov(v2, 1, index) || !mov(v3, 2, value)
            || index < 0 || index >= ::hojy::content::TeamMemberCount) {
            return fault("event team member index out of range");
        }
        ::hojy::world::state::gSaveData.baseInfo->members[index] = value;
        return completed();
    }
    case 19: {
        std::int16_t index = 0;
        if (!mov(v2, 1, index) || index < 0 || index >= ::hojy::content::TeamMemberCount
            || !write(v3, ::hojy::world::state::gSaveData.baseInfo->members[index])) {
            return fault("event team member read out of range");
        }
        return completed();
    }
    case 20: {
        std::int16_t item = 0;
        if (!mov(v2, 1, item) || item < 0 || !write(v3, ::hojy::world::state::gBag[item])) {
            return fault("event inventory read out of range");
        }
        return completed();
    }
    case 21: {
        std::int16_t mapIndex = 0, eventIndex = 0, wordIndex = 0, value = 0;
        if (!mov(v2, 1, mapIndex) || !mov(v3, 2, eventIndex)
            || !mov(v4, 4, wordIndex) || !mov(v5, 8, value)
            || mapIndex < 0 || static_cast<std::size_t>(mapIndex) >= ::hojy::world::state::gSaveData.subMapEventInfo.size()
            || eventIndex < 0 || eventIndex >= ::hojy::content::SubMapEventCount
            || wordIndex < 0 || wordIndex > 10) {
            return fault("event sub-map event index out of range");
        }
        auto &eventData = ::hojy::world::state::gSaveData.subMapEventInfo[mapIndex]->events[eventIndex];
        return writeSubMapEventWord(eventData, wordIndex, value)
            ? completed() : fault("event sub-map event field is invalid");
    }
    case 22: {
        std::int16_t mapIndex = 0, eventIndex = 0, wordIndex = 0, destination = v5;
        if (!mov(v2, 1, mapIndex) || !mov(v3, 2, eventIndex)
            || !mov(v4, 4, wordIndex)
            || mapIndex < 0 || static_cast<std::size_t>(mapIndex) >= ::hojy::world::state::gSaveData.subMapEventInfo.size()
            || eventIndex < 0 || eventIndex >= ::hojy::content::SubMapEventCount
            || wordIndex < 0 || wordIndex > 10) {
            return fault("event sub-map event index out of range");
        }
        const auto &eventData = ::hojy::world::state::gSaveData.subMapEventInfo[mapIndex]->events[eventIndex];
        std::int16_t value = 0;
        return readSubMapEventWord(eventData, wordIndex, value) && write(destination, value)
            ? completed() : fault("event sub-map event field is invalid");
    }
    case 23: {
        std::int16_t mapIndex = 0, layer = 0, x = 0, y = 0, value = 0;
        if (!mov(v2, 1, mapIndex) || !mov(v3, 2, layer) || !mov(v4, 4, x)
            || !mov(v5, 8, y) || !mov(v6, 16, value)
            || mapIndex < 0 || static_cast<std::size_t>(mapIndex) >= ::hojy::world::state::gSaveData.subMapLayerInfo.size()
            || layer < 0 || layer >= ::hojy::content::SubMapLayerCount
            || x < 0 || x >= ::hojy::content::SubMapWidth || y < 0 || y >= ::hojy::content::SubMapHeight) {
            return fault("event sub-map layer index out of range");
        }
        ::hojy::world::state::gSaveData.subMapLayerInfo[mapIndex]->data[layer][x + y * ::hojy::content::SubMapWidth] = value;
        if (mapIndex == subMapId_) { setCellTexture(x, y, layer, value >> 1); drawDirty_ = true; }
        return completed();
    }
    case 24: {
        std::int16_t mapIndex = 0, layer = 0, x = 0, y = 0;
        if (!mov(v2, 1, mapIndex) || !mov(v3, 2, layer) || !mov(v4, 4, x)
            || !mov(v5, 8, y)
            || mapIndex < 0 || static_cast<std::size_t>(mapIndex) >= ::hojy::world::state::gSaveData.subMapLayerInfo.size()
            || layer < 0 || layer >= ::hojy::content::SubMapLayerCount
            || x < 0 || x >= ::hojy::content::SubMapWidth || y < 0 || y >= ::hojy::content::SubMapHeight
            || !write(v6, ::hojy::world::state::gSaveData.subMapLayerInfo[mapIndex]->data[layer][x + y * ::hojy::content::SubMapWidth])) {
            return fault("event sub-map layer read out of range");
        }
        return completed();
    }
    case 25:
    case 26:
        return fault("direct event address access is unsupported");
    case 27: {
        std::int16_t index = 0;
        if (!mov(v3, 1, index)) { return fault("event name index out of range"); }
        const char *name = nullptr;
        std::size_t length = 0;
        switch (v2) {
        case 0: if (auto *p = ::hojy::world::state::gSaveData.charInfo[index]) { name = p->name; length = sizeof(p->name); } break;
        case 1: if (auto *p = ::hojy::world::state::gSaveData.itemInfo[index]) { name = p->name; length = sizeof(p->name); } break;
        case 2: if (auto *p = ::hojy::world::state::gSaveData.subMapInfo[index]) { name = p->name; length = sizeof(p->name); } break;
        case 3: if (auto *p = ::hojy::world::state::gSaveData.skillInfo[index]) { name = p->name; length = sizeof(p->name); } break;
        default: return fault("unknown event name table");
        }
        if (!name) {
            return fault("event name exceeds memory");
        }
        std::vector<char> bytes(length + 1, '\0');
        std::memcpy(bytes.data(), name, length);
        if (!memory.writeBytes(v4, 0, bytes.data(), bytes.size())) {
            return fault("event name exceeds memory");
        }
        return completed();
    }
    case 32: {
        std::int16_t index = 0, value = 0;
        if (!mov(v3, 1, index) || !read(v2, value)
            || index < 0
            || !eventVm_.patchLegacyRelative(index, value)) {
            return fault("event list write out of range");
        }
        return completed();
    }
    case 33: {
        std::int16_t x = 0, y = 0, color = 0;
        if (!mov(v3, 1, x) || !mov(v4, 2, y) || !mov(v5, 4, color)) {
            return fault("event text position out of range");
        }
        const auto text = memory.readCString(v2);
        if (!text) { return fault("event text is not terminated"); }
        auto str = util::big5Conv.toUnicode(text->c_str());
        transformOffset(x, y, gWindow->width(), gWindow->height());
        ensureExtendedNode();
        extendedNode_->addText(x, y, str, color & 0xFF, color >> 8);
        return completed();
    }
    case 34: {
        std::int16_t x0 = 0, y0 = 0, x1 = 0, y1 = 0;
        if (!mov(v2, 1, x0) || !mov(v3, 2, y0) || !mov(v4, 4, x1) || !mov(v5, 8, y1)) {
            return fault("event box position out of range");
        }
        transformOffset(x0, y0, gWindow->width(), gWindow->height());
        transformOffset(x1, y1, gWindow->width(), gWindow->height());
        ensureExtendedNode();
        extendedNode_->addBox(x0, y0, x1, y1);
        return completed();
    }
    case 35: {
        ensureExtendedNode();
        auto *node = extendedNode_;
        node->setWaitForKeyPress();
        node->setHandler([this, node, v1] {
            std::int16_t value = 0;
            switch (node->keyPressed()) {
            case Node::KeyLeft: value = 154; break;
            case Node::KeyRight: value = 156; break;
            case Node::KeyUp: value = 158; break;
            case Node::KeyDown: value = 152; break;
            default: break;
            }
            (void)eventVm_.memory().writeWord(v1, value);
            continueEvents(false);
        });
        return waiting();
    }
    case 36: {
        std::int16_t x = 0, y = 0, color = 0;
        if (!mov(v3, 1, x) || !mov(v4, 2, y) || !mov(v5, 4, color)) {
            return fault("event text position out of range");
        }
        const auto text = memory.readCString(v2);
        if (!text) { return fault("event text is not terminated"); }
        transformOffset(x, y, gWindow->width(), gWindow->height());
        ensureExtendedNode();
        auto *node = extendedNode_;
        node->addText(x, y, util::big5Conv.toUnicode(text->c_str()), color & 0xFF, color >> 8);
        node->setWaitForKeyPress();
        node->setHandler([this, node] {
            (void)eventVm_.memory().writeWord(0x7000, node->keyPressed() == Node::KeyOK ? 0 : 1);
            continueEvents(false);
        });
        return waiting();
    }
    case 37: {
        std::int16_t millisec = 0;
        if (!mov(v2, 1, millisec) || millisec < 0) {
            return fault("event timeout out of range");
        }
        ensureExtendedNode();
        extendedNode_->setTimeToClose(static_cast<std::uint32_t>(millisec));
        extendedNode_->setHandler([this] { continueEvents(false); });
        return waiting();
    }
    case 38: {
        std::int16_t range = 0;
        if (!mov(v2, 1, range) || range < 0 || !write(v3, util::gRandom(range))) {
            return fault("event random operand out of range");
        }
        return completed();
    }
    case 39:
    case 40: {
        std::int16_t count = 0, x = 0, y = 0;
        if (!mov(v2, 1, count) || !mov(v5, 2, x) || !mov(v6, 4, y) || count < 0 || count > 256) {
            return fault("event menu operand out of range");
        }
        std::vector<std::wstring> strings;
        strings.reserve(static_cast<std::size_t>(count));
        for (std::int16_t i = 0; i < count; ++i) {
            const auto text = memory.readCString(static_cast<std::int32_t>(v3) + i);
            if (!text) { return fault("event menu string is not terminated"); }
            strings.emplace_back(util::big5Conv.toUnicode(text->c_str()));
        }
        transformOffset(x, y, gWindow->width(), gWindow->height());
        auto *menu = new MenuTextList(this, x, y, gWindow->width() - x, gWindow->height() - y);
        menu->setHandler([this, v4, menu] {
            (void)eventVm_.memory().writeWord(v4, static_cast<std::int16_t>(menu->currIndex() + 1));
            menu->requestDelete();
        }, [this, v4]() -> bool {
            (void)eventVm_.memory().writeWord(v4, 0);
            return true;
        });
        menu->popup(strings);
        return completed();
    }
    case 41: {
        std::int16_t kind = 0, x = 0, y = 0, id = 0;
        if (!mov(v3, 1, x) || !mov(v4, 2, y) || !mov(v5, 4, id)) {
            return fault("event texture operand out of range");
        }
        transformOffset(x, y, gWindow->width(), gWindow->height());
        ensureExtendedNode();
        if (v2 == 0) {
            extendedNode_->addTexture(x, y, gWindow->smpTexture(id), util::calcSmallestDivision(gWindow->width(), OrigWidth));
        } else if (v2 == 1) {
            extendedNode_->addTexture(x, y, gWindow->headTexture(id), util::calcSmallestDivision(gWindow->width(), OrigWidth));
        } else {
            return fault("unknown event texture table");
        }
        (void)kind;
        return completed();
    }
    case 42: {
        std::int16_t x = 0, y = 0;
        if (!mov(v2, 1, x) || !mov(v3, 1, y)) { return fault("event map position out of range"); }
        if (!gWindow->globalMap()) { return fault("global map is unavailable"); }
        gWindow->globalMap()->setPosition(x, y, false);
        return completed();
    }
    case 43: {
        std::int16_t eventId = 0, a = 0, b = 0, c = 0, d = 0;
        if (!mov(v2, 1, eventId) || !mov(v3, 2, a) || !mov(v4, 4, b)
            || !mov(v5, 8, c) || !mov(v6, 16, d)
            || !write(0x7100, a) || !write(0x7101, b)
            || !write(0x7102, c) || !write(0x7103, d)) {
            return fault("event chained event operand out of range");
        }
        runEvent(eventId);
        return completed();
    }
    case 48: {
        std::int32_t end = static_cast<std::int32_t>(v1) + v2;
        if (end < event::EventMemory::MinWordAddress || end > event::EventMemory::MaxWordAddress) {
            return fault("event debug range out of bounds");
        }
        for (std::int32_t address = v1; address < end; ++address) {
            std::int16_t value = 0;
            if (!read(address, value)) { return fault("event debug read out of range"); }
            fmt::print(stdout, "RAM[0x{:04X}] = {}\n", address, value);
        }
        return completed();
    }
    case 52: {
        std::int16_t charId = 0, skillIndex = 0, required = 0;
        if (!mov(v2, 1, charId) || !mov(v3, 2, skillIndex) || !mov(v4, 4, required)
            || charId < 0 || skillIndex < 0 || skillIndex >= ::hojy::content::LearnSkillCount) {
            return fault("event skill operand out of range");
        }
        auto *character = ::hojy::world::state::gSaveData.charInfo[charId];
        if (!character || !write(0x7000,
                std::clamp<std::int16_t>(character->skillLevel[skillIndex] / 100, 0, 9) + 1 >= required ? 0 : 1)) {
            return fault("event skill lookup failed");
        }
        return completed();
    }
    default:
        return fault("unsupported extended event opcode");
    }
}

}
