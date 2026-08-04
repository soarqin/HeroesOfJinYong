#pragma once

#include "event_memory.hh"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace hojy::event {

struct Instruction {
    std::int16_t opcode = 0;
    std::array<std::int16_t, 6> operands{};
};

enum class VmStatus {
    Running,
    Waiting,
    Completed,
    Faulted,
};

struct VmResult {
    VmStatus status = VmStatus::Running;
    std::size_t executed = 0;
    std::string error;
};

struct LegacyInstruction {
    std::int16_t opcode = 0;
    std::vector<std::int16_t> operands;
    std::size_t wordOffset = 0;
    std::size_t nextWordOffset = 0;
    bool conditional = false;
    std::size_t trueAdvance = 0;
    std::size_t falseAdvance = 0;
};

struct LegacyHostResult {
    VmStatus status = VmStatus::Running;
    bool branch = false;
    std::string error;
};

class VmHost {
public:
    virtual ~VmHost() = default;
    virtual VmResult execute(const Instruction &instruction,
                             EventMemory &memory) = 0;
};

class LegacyVmHost {
public:
    virtual ~LegacyVmHost() = default;
    virtual bool decodeLegacy(const std::vector<std::int16_t> &program,
                              std::size_t programCounter,
                              LegacyInstruction &instruction,
                              std::string &error) const = 0;
    virtual LegacyHostResult executeLegacy(
            const LegacyInstruction &instruction,
            EventMemory &memory) = 0;
};

class Vm final {
public:
    void load(std::vector<Instruction> program);
    void loadLegacy(std::vector<std::int16_t> program);
    void reset();

    [[nodiscard]] VmResult run(VmHost &host, std::size_t operationBudget);
    [[nodiscard]] VmResult step(const Instruction &instruction, VmHost &host);
    [[nodiscard]] VmResult runLegacy(LegacyVmHost &host,
                                     std::size_t operationBudget);
    bool resumeLegacy(bool branch);
    bool patchLegacyRelative(std::ptrdiff_t offset, std::int16_t value);
    [[nodiscard]] std::size_t programCounter() const { return programCounter_; }
    [[nodiscard]] std::size_t legacyProgramCounter() const {
        return legacyProgramCounter_;
    }
    [[nodiscard]] bool legacyActive() const { return legacyActive_; }
    [[nodiscard]] bool legacyWaiting() const { return legacyWaiting_; }
    [[nodiscard]] bool legacyDispatching() const { return legacyDispatching_; }
    [[nodiscard]] const EventMemory &memory() const { return memory_; }
    [[nodiscard]] EventMemory &memory() { return memory_; }

private:
    enum class PureResult {
        Handled,
        Host,
        Faulted,
    };

    PureResult executePure(const Instruction &instruction, std::string &error);
    bool resolve(std::int16_t flags, std::int16_t operand, std::int16_t bit,
                 std::int16_t &value, std::string &error) const;
    bool addressAdd(std::int16_t left, std::int16_t right,
                    std::int32_t &address, std::string &error) const;
    bool applyLegacyAdvance(std::size_t advance);
    void clearLegacyExecutionState();

    EventMemory memory_;
    std::vector<Instruction> program_;
    std::size_t programCounter_ = 0;
    std::vector<std::int16_t> legacyProgram_;
    std::size_t legacyProgramCounter_ = 0;
    std::size_t legacyInstructionNext_ = 0;
    std::size_t legacyTrueAdvance_ = 0;
    std::size_t legacyFalseAdvance_ = 0;
    bool legacyActive_ = false;
    bool legacyWaiting_ = false;
    bool legacyConditionalWait_ = false;
    bool legacyDispatching_ = false;
};

}
