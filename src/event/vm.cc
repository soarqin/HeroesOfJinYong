#include "vm.hh"

#include <cstring>
#include <limits>
#include <utility>

namespace hojy::event {

void Vm::load(std::vector<Instruction> program) {
    program_ = std::move(program);
    reset();
}

void Vm::loadLegacy(std::vector<std::int16_t> program) {
    legacyProgram_ = std::move(program);
    legacyProgramCounter_ = 0;
    legacyInstructionNext_ = 0;
    legacyTrueAdvance_ = 0;
    legacyFalseAdvance_ = 0;
    legacyActive_ = !legacyProgram_.empty();
    legacyWaiting_ = false;
    legacyConditionalWait_ = false;
}

void Vm::reset() {
    programCounter_ = 0;
    memory_.clear();
    legacyProgram_.clear();
    clearLegacyExecutionState();
}

void Vm::clearLegacyExecutionState() {
    legacyProgramCounter_ = 0;
    legacyInstructionNext_ = 0;
    legacyTrueAdvance_ = 0;
    legacyFalseAdvance_ = 0;
    legacyActive_ = false;
    legacyWaiting_ = false;
    legacyConditionalWait_ = false;
    legacyDispatching_ = false;
}

bool Vm::applyLegacyAdvance(std::size_t advance) {
    if (legacyProgramCounter_ > legacyProgram_.size()
        || advance > legacyProgram_.size() - legacyProgramCounter_) {
        legacyActive_ = false;
        legacyWaiting_ = false;
        return false;
    }
    legacyProgramCounter_ += advance;
    if (legacyProgramCounter_ == legacyProgram_.size()) {
        legacyActive_ = false;
    }
    return true;
}

VmResult Vm::runLegacy(LegacyVmHost &host, std::size_t operationBudget) {
    if (!legacyActive_) {
        return {VmStatus::Completed, 0, {}};
    }
    if (legacyWaiting_) {
        return {VmStatus::Waiting, 0, {}};
    }
    if (operationBudget == 0) {
        return {VmStatus::Running, 0, {}};
    }

    std::size_t executed = 0;
    while (legacyActive_ && executed < operationBudget) {
        if (legacyProgramCounter_ >= legacyProgram_.size()) {
            legacyActive_ = false;
            return {VmStatus::Completed, executed, {}};
        }

        LegacyInstruction instruction;
        std::string error;
        if (!host.decodeLegacy(legacyProgram_, legacyProgramCounter_,
                               instruction, error)) {
            legacyActive_ = false;
            legacyWaiting_ = false;
            return {VmStatus::Faulted, executed, std::move(error)};
        }
        if (instruction.wordOffset != legacyProgramCounter_
            || instruction.nextWordOffset <= legacyProgramCounter_
            || instruction.nextWordOffset > legacyProgram_.size()
            || (instruction.conditional
                && (instruction.trueAdvance
                        > legacyProgram_.size() - instruction.nextWordOffset
                    || instruction.falseAdvance
                        > legacyProgram_.size() - instruction.nextWordOffset))) {
            legacyActive_ = false;
            legacyWaiting_ = false;
            return {VmStatus::Faulted, executed,
                    "invalid legacy event instruction boundary"};
        }

        legacyInstructionNext_ = instruction.nextWordOffset;
        legacyProgramCounter_ = instruction.nextWordOffset;
        legacyDispatching_ = true;
        auto result = host.executeLegacy(instruction, memory_);
        legacyDispatching_ = false;
        ++executed;

        if (result.status == VmStatus::Faulted) {
            legacyActive_ = false;
            legacyWaiting_ = false;
            return {VmStatus::Faulted, executed, std::move(result.error)};
        }
        if (result.status == VmStatus::Completed) {
            legacyActive_ = false;
            legacyWaiting_ = false;
            legacyProgramCounter_ = legacyProgram_.size();
            return {VmStatus::Completed, executed, {}};
        }

        if (instruction.conditional) {
            if (result.status == VmStatus::Waiting) {
                legacyTrueAdvance_ = instruction.trueAdvance;
                legacyFalseAdvance_ = instruction.falseAdvance;
                legacyConditionalWait_ = true;
            } else if (!applyLegacyAdvance(
                           result.branch ? instruction.trueAdvance
                                         : instruction.falseAdvance)) {
                return {VmStatus::Faulted, executed,
                        "legacy event branch target out of range"};
            }
        } else if (result.status == VmStatus::Waiting) {
            legacyConditionalWait_ = false;
        }

        if (result.status == VmStatus::Waiting) {
            legacyWaiting_ = true;
            return {VmStatus::Waiting, executed, {}};
        }
        if (!legacyActive_) {
            return {VmStatus::Completed, executed, {}};
        }
    }
    return legacyActive_ ? VmResult{VmStatus::Running, executed, {}}
                         : VmResult{VmStatus::Completed, executed, {}};
}

bool Vm::resumeLegacy(bool branch) {
    if (!legacyActive_ || !legacyWaiting_) {
        return false;
    }
    legacyWaiting_ = false;
    if (legacyConditionalWait_) {
        const auto advance = branch ? legacyTrueAdvance_ : legacyFalseAdvance_;
        legacyConditionalWait_ = false;
        legacyTrueAdvance_ = 0;
        legacyFalseAdvance_ = 0;
        return applyLegacyAdvance(advance);
    }
    if (legacyProgramCounter_ == legacyProgram_.size()) {
        legacyActive_ = false;
    }
    return true;
}

bool Vm::patchLegacyRelative(std::ptrdiff_t offset, std::int16_t value) {
    if (!legacyDispatching_) {
        return false;
    }
    const auto base = static_cast<std::ptrdiff_t>(legacyInstructionNext_);
    if ((offset > 0 && base > std::numeric_limits<std::ptrdiff_t>::max() - offset)
        || (offset < 0 && base < std::numeric_limits<std::ptrdiff_t>::min() - offset)) {
        return false;
    }
    const auto target = base + offset;
    if (target < 0
        || static_cast<std::size_t>(target) >= legacyProgram_.size()) {
        return false;
    }
    legacyProgram_[static_cast<std::size_t>(target)] = value;
    return true;
}

bool Vm::resolve(std::int16_t flags, std::int16_t operand, std::int16_t bit,
                 std::int16_t &value, std::string &error) const {
    if ((flags & bit) == 0) {
        value = operand;
        return true;
    }
    if (!memory_.readWord(operand, value)) {
        error = "event memory read out of range";
        return false;
    }
    return true;
}

bool Vm::addressAdd(std::int16_t left, std::int16_t right,
                    std::int32_t &address, std::string &error) const {
    address = static_cast<std::int32_t>(left) + static_cast<std::int32_t>(right);
    if (address < EventMemory::MinWordAddress || address > EventMemory::MaxWordAddress) {
        error = "event memory address overflow";
        return false;
    }
    return true;
}

Vm::PureResult Vm::executePure(const Instruction &instruction, std::string &error) {
    const auto v0 = instruction.opcode;
    const auto v1 = instruction.operands[0];
    const auto v2 = instruction.operands[1];
    const auto v3 = instruction.operands[2];
    const auto v4 = instruction.operands[3];
    const auto v5 = instruction.operands[4];

    switch (v0) {
    case 0:
        return memory_.writeWord(v1, v2) ? PureResult::Handled :
            (error = "event memory write out of range", PureResult::Faulted);
    case 1: {
        std::int16_t offset = 0, value = 0;
        if (!resolve(v1, v4, 1, offset, error) || !resolve(v1, v4, 2, value, error)) {
            return PureResult::Faulted;
        }
        std::int32_t address = 0;
        if (!addressAdd(v3, offset, address, error)) {
            return PureResult::Faulted;
        }
        if (v2) { value &= 0xFF; }
        return memory_.writeWord(address, value) ? PureResult::Handled :
            (error = "event memory write out of range", PureResult::Faulted);
    }
    case 2: {
        std::int16_t oldValue = 0, offset = 0, ignored = 0;
        if (!memory_.readWord(v5, oldValue) || !resolve(v1, v4, 1, offset, error)) {
            if (error.empty()) { error = "event memory read out of range"; }
            return PureResult::Faulted;
        }
        std::int32_t address = 0;
        if (!addressAdd(v3, offset, address, error) || !memory_.readWord(address, ignored)) {
            if (error.empty()) { error = "event memory read out of range"; }
            return PureResult::Faulted;
        }
        if (v2) { oldValue &= 0xFF; }
        return memory_.writeWord(v5, oldValue) ? PureResult::Handled :
            (error = "event memory write out of range", PureResult::Faulted);
    }
    case 3: {
        std::int16_t value = 0, lhs = 0;
        if (!resolve(v1, v5, 1, value, error) || !memory_.readWord(v4, lhs)) {
            if (error.empty()) { error = "event memory read out of range"; }
            return PureResult::Faulted;
        }
        std::int32_t result = lhs;
        switch (v2) {
        case 0: result = static_cast<std::int32_t>(lhs) + value; break;
        case 1: result = static_cast<std::int32_t>(lhs) - value; break;
        case 2: result = static_cast<std::int32_t>(lhs) * value; break;
        case 3: if (value != 0) { result = lhs / value; } break;
        case 4: if (value != 0) { result = lhs % value; } break;
        default: break;
        }
        return memory_.writeWord(v3, static_cast<std::int16_t>(result)) ? PureResult::Handled :
            (error = "event memory write out of range", PureResult::Faulted);
    }
    case 4: {
        std::int16_t value = 0, lhs = 0;
        if (!resolve(v1, v4, 1, value, error) || !memory_.readWord(v3, lhs)) {
            if (error.empty()) { error = "event memory read out of range"; }
            return PureResult::Faulted;
        }
        std::int16_t result = 0;
        switch (v2) {
        case 0: result = lhs < value ? 0 : 1; break;
        case 1: result = lhs <= value ? 0 : 1; break;
        case 2: result = lhs == value ? 0 : 1; break;
        case 3: result = lhs != value ? 0 : 1; break;
        case 4: result = lhs >= value ? 0 : 1; break;
        case 5: result = lhs > value ? 0 : 1; break;
        case 6: result = 0; break;
        case 7: result = 1; break;
        default: break;
        }
        return memory_.writeWord(0x7000, result) ? PureResult::Handled :
            (error = "event memory write out of range", PureResult::Faulted);
    }
    case 5:
        memory_.clear();
        return PureResult::Handled;
    case 9: {
        std::int16_t value = 0;
        if (!resolve(v1, v4, 1, value, error)) { return PureResult::Faulted; }
        const auto format = memory_.readCString(v3);
        if (!format || format->find('%') == std::string::npos) {
            error = "invalid bounded event format string";
            return PureResult::Faulted;
        }
        // Keep the VM boundary strict; host code can provide richer formatting.
        error = "formatted event strings require a host adapter";
        return PureResult::Host;
    }
    case 10: {
        const auto value = memory_.readCString(v1);
        if (!value || !memory_.writeWord(v2, static_cast<std::int16_t>(
                std::min<std::size_t>(value->size(), std::numeric_limits<std::int16_t>::max())))) {
            error = "invalid bounded event string";
            return PureResult::Faulted;
        }
        return PureResult::Handled;
    }
    case 11: {
        const auto suffix = memory_.readCString(v2);
        if (!suffix || !memory_.appendCString(v1, *suffix)) {
            error = "invalid bounded event string append";
            return PureResult::Faulted;
        }
        return PureResult::Handled;
    }
    case 12: {
        std::int16_t length = 0;
        if (!resolve(v1, v3, 1, length, error) || length < 0) {
            if (error.empty()) { error = "invalid event string length"; }
            return PureResult::Faulted;
        }
        std::vector<std::uint8_t> spaces(static_cast<std::size_t>(length) + 1, 0x20);
        spaces.back() = 0;
        if (!memory_.writeBytes(v2, 0, spaces.data(), spaces.size())) {
            error = "event string fill out of range";
            return PureResult::Faulted;
        }
        return PureResult::Handled;
    }
    default:
        return PureResult::Host;
    }
}

VmResult Vm::run(VmHost &host, std::size_t operationBudget) {
    if (programCounter_ >= program_.size()) {
        return {VmStatus::Completed, 0, {}};
    }
    if (operationBudget == 0) {
        return {VmStatus::Running, 0, {}};
    }
    std::size_t executed = 0;
    while (programCounter_ < program_.size() && executed < operationBudget) {
        const auto &instruction = program_[programCounter_];
        std::string error;
        const auto pure = executePure(instruction, error);
        if (pure == PureResult::Faulted) {
            return {VmStatus::Faulted, executed, std::move(error)};
        }
        ++programCounter_;
        ++executed;
        if (pure == PureResult::Host) {
            auto result = host.execute(instruction, memory_);
            result.executed += executed;
            if (result.status == VmStatus::Waiting || result.status == VmStatus::Faulted) {
                return result;
            }
            if (result.status == VmStatus::Completed) {
                programCounter_ = program_.size();
                return result;
            }
        }
    }
    if (programCounter_ >= program_.size()) {
        return {VmStatus::Completed, executed, {}};
    }
    return {VmStatus::Running, executed, {}};
}

VmResult Vm::step(const Instruction &instruction, VmHost &host) {
    std::string error;
    const auto pure = executePure(instruction, error);
    if (pure == PureResult::Faulted) {
        return {VmStatus::Faulted, 0, std::move(error)};
    }
    if (pure == PureResult::Host) {
        return host.execute(instruction, memory_);
    }
    return {VmStatus::Completed, 1, {}};
}

}
