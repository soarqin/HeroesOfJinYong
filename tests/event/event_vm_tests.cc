#include "event/event_memory.hh"
#include "event/vm.hh"

#include "test_support.hh"

#include <iostream>
#include <string>
#include <vector>

namespace {

class WaitingHost final: public hojy::event::VmHost {
public:
    hojy::event::VmResult execute(const hojy::event::Instruction &instruction,
                                  hojy::event::EventMemory &) override {
        if (instruction.opcode == 35) {
            return {hojy::event::VmStatus::Waiting, 0, {}};
        }
        return {hojy::event::VmStatus::Faulted, 0, "unsupported host opcode"};
    }
};

class LegacyHost final: public hojy::event::LegacyVmHost {
public:
    explicit LegacyHost(hojy::event::Vm *vm = nullptr): vm_(vm) {
    }

    bool decodeLegacy(const std::vector<std::int16_t> &program,
                      std::size_t programCounter,
                      hojy::event::LegacyInstruction &instruction,
                      std::string &error) const override {
        if (programCounter >= program.size()) {
            error = "legacy event program counter out of range";
            return false;
        }
        instruction = {};
        instruction.wordOffset = programCounter;
        instruction.opcode = program[programCounter];
        instruction.conditional = instruction.opcode == 1;
        const std::size_t wordCount = instruction.conditional ? 4 : 2;
        if (wordCount > program.size() - programCounter) {
            error = "truncated legacy event instruction";
            return false;
        }
        instruction.operands.push_back(program[programCounter + 1]);
        if (instruction.conditional) {
            instruction.trueAdvance = static_cast<std::size_t>(program[programCounter + 2]);
            instruction.falseAdvance = static_cast<std::size_t>(program[programCounter + 3]);
        }
        instruction.nextWordOffset = programCounter + wordCount;
        return true;
    }

    hojy::event::LegacyHostResult executeLegacy(
            const hojy::event::LegacyInstruction &instruction,
            hojy::event::EventMemory &) override {
        calls.push_back(instruction.opcode);
        values.push_back(instruction.operands.front());
        if (instruction.opcode == 1 || instruction.opcode == 4) {
            return {hojy::event::VmStatus::Waiting, false, {}};
        }
        if (instruction.opcode == 6 && vm_
            && !vm_->patchLegacyRelative(1, 99)) {
            return {hojy::event::VmStatus::Faulted, false,
                    "failed to patch next legacy operand"};
        }
        return {hojy::event::VmStatus::Running, false, {}};
    }

    std::vector<std::int16_t> calls;
    std::vector<std::int16_t> values;

private:
    hojy::event::Vm *vm_ = nullptr;
};

void testEventMemoryChecksSignedAddressBoundsTransactionally() {
    hojy::event::EventMemory memory;
    HOJY_CHECK_EQ(memory.writeWord(-32768, 123), true);
    HOJY_CHECK_EQ(memory.writeWord(32767, 456), true);
    std::int16_t value = 0;
    HOJY_CHECK_EQ(memory.readWord(-32768, value), true);
    HOJY_CHECK_EQ(value, 123);
    HOJY_CHECK_EQ(memory.writeWord(32768, 999), false);
    HOJY_CHECK_EQ(memory.readWord(-32768, value), true);
    HOJY_CHECK_EQ(value, 123);
}

void testEventMemoryBoundsCStringWritesAndAppends() {
    hojy::event::EventMemory memory;
    HOJY_CHECK_EQ(memory.writeCString(100, "hello"), true);
    HOJY_CHECK_EQ(memory.appendCString(100, " world"), true);
    const auto text = memory.readCString(100);
    HOJY_CHECK_EQ(text.has_value(), true);
    HOJY_CHECK_EQ(*text, std::string("hello world"));

    HOJY_CHECK_EQ(memory.writeCString(32767, "too long"), false);
    std::int16_t value = 7;
    HOJY_CHECK_EQ(memory.readWord(32767, value), true);
    HOJY_CHECK_EQ(value, 0);
}

void testVmRunsPureInstructionsAndRespectsBudget() {
    hojy::event::Vm vm;
    vm.load({
        {0, {10, 7, 0, 0, 0, 0}},
        {3, {0, 0, 12, 10, 5, 0}},
        {4, {0, 2, 12, 12, 0, 0}},
    });
    WaitingHost host;
    const auto first = vm.run(host, 2);
    HOJY_CHECK_EQ(first.status, hojy::event::VmStatus::Running);
    HOJY_CHECK_EQ(vm.programCounter(), 2U);
    const auto second = vm.run(host, 2);
    HOJY_CHECK_EQ(second.status, hojy::event::VmStatus::Completed);
    std::int16_t value = 0;
    HOJY_CHECK_EQ(vm.memory().readWord(12, value), true);
    HOJY_CHECK_EQ(value, 12);
    HOJY_CHECK_EQ(vm.memory().readWord(0x7000, value), true);
    HOJY_CHECK_EQ(value, 0);
}

void testVmWaitsAndResumesAfterHostInstruction() {
    hojy::event::Vm vm;
    vm.load({
        {35, {0, 0, 0, 0, 0, 0}},
        {0, {20, 99, 0, 0, 0, 0}},
    });
    WaitingHost host;
    const auto waiting = vm.run(host, 8);
    HOJY_CHECK_EQ(waiting.status, hojy::event::VmStatus::Waiting);
    HOJY_CHECK_EQ(vm.programCounter(), 1U);
    const auto completed = vm.run(host, 8);
    HOJY_CHECK_EQ(completed.status, hojy::event::VmStatus::Completed);
}

void testVmFaultsOnOutOfRangeMemoryWithoutPartialWrite() {
    hojy::event::Vm vm;
    vm.load({{0, {32767, 11, 0, 0, 0, 0}},
             {11, {32767, 32767, 0, 0, 0, 0}}});
    WaitingHost host;
    const auto result = vm.run(host, 8);
    HOJY_CHECK_EQ(result.status, hojy::event::VmStatus::Faulted);
    std::int16_t value = 0;
    HOJY_CHECK_EQ(vm.memory().readWord(32767, value), true);
    HOJY_CHECK_EQ(value, 11);
}

void testLegacyVmWaitsAndResumesConditionalBranches() {
    hojy::event::Vm vm;
    LegacyHost host;
    vm.loadLegacy({
        1, 7, 2, 0,
        2, 20,
        3, 30,
    });

    const auto waiting = vm.runLegacy(host, 8);
    HOJY_CHECK_EQ(waiting.status, hojy::event::VmStatus::Waiting);
    HOJY_CHECK_EQ(vm.legacyWaiting(), true);
    HOJY_CHECK_EQ(vm.legacyProgramCounter(), 4U);
    HOJY_CHECK_EQ(host.calls.size(), 1U);

    HOJY_CHECK_EQ(vm.resumeLegacy(true), true);
    HOJY_CHECK_EQ(vm.legacyProgramCounter(), 6U);
    const auto completed = vm.runLegacy(host, 8);
    HOJY_CHECK_EQ(completed.status, hojy::event::VmStatus::Completed);
    HOJY_CHECK_EQ(host.calls.size(), 2U);
    HOJY_CHECK_EQ(host.calls[1], 3);
}

void testLegacyVmSequentialWaitIgnoresResumeResult() {
    hojy::event::Vm vm;
    LegacyHost host;
    vm.loadLegacy({4, 11, 5, 12});

    HOJY_CHECK_EQ(vm.runLegacy(host, 8).status,
                  hojy::event::VmStatus::Waiting);
    HOJY_CHECK_EQ(vm.resumeLegacy(true), true);
    HOJY_CHECK_EQ(vm.legacyProgramCounter(), 2U);
    HOJY_CHECK_EQ(vm.runLegacy(host, 8).status,
                  hojy::event::VmStatus::Completed);
    HOJY_CHECK_EQ(host.calls.size(), 2U);
    HOJY_CHECK_EQ(host.calls[0], 4);
    HOJY_CHECK_EQ(host.calls[1], 5);
}

void testLegacyVmRespectsBudgetAndPatchesRelativeToNextInstruction() {
    hojy::event::Vm vm;
    LegacyHost host(&vm);
    vm.loadLegacy({6, 1, 7, 2, 8, 3});

    const auto first = vm.runLegacy(host, 2);
    HOJY_CHECK_EQ(first.status, hojy::event::VmStatus::Running);
    HOJY_CHECK_EQ(first.executed, 2U);
    HOJY_CHECK_EQ(vm.legacyProgramCounter(), 4U);
    HOJY_CHECK_EQ(host.values.size(), 2U);
    HOJY_CHECK_EQ(host.values[1], 99);

    const auto second = vm.runLegacy(host, 2);
    HOJY_CHECK_EQ(second.status, hojy::event::VmStatus::Completed);
    HOJY_CHECK_EQ(host.calls.size(), 3U);
}

void testLegacyVmFaultsBeforeExecutingTruncatedInstruction() {
    hojy::event::Vm vm;
    LegacyHost host;
    vm.loadLegacy({1, 7, 2});

    const auto result = vm.runLegacy(host, 8);
    HOJY_CHECK_EQ(result.status, hojy::event::VmStatus::Faulted);
    HOJY_CHECK_EQ(host.calls.empty(), true);
    HOJY_CHECK_EQ(vm.legacyActive(), false);
}

}

int main() {
    try {
        testEventMemoryChecksSignedAddressBoundsTransactionally();
        testEventMemoryBoundsCStringWritesAndAppends();
        testVmRunsPureInstructionsAndRespectsBudget();
        testVmWaitsAndResumesAfterHostInstruction();
        testVmFaultsOnOutOfRangeMemoryWithoutPartialWrite();
        testLegacyVmWaitsAndResumesConditionalBranches();
        testLegacyVmSequentialWaitIgnoresResumeResult();
        testLegacyVmRespectsBudgetAndPatchesRelativeToNextInstruction();
        testLegacyVmFaultsBeforeExecutingTruncatedInstruction();
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
