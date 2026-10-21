#include "mapwithevent.hh"

#include "window_command.hh"
#include "content/event.hh"

#include <cstdio>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace hojy::scene {
namespace {

constexpr std::size_t LegacyEventOperationBudget = 1024;

template <typename T>
struct LegacyHandlerTraits;

template <typename R, typename P, typename... Args>
struct LegacyHandlerTraits<R (*)(P, Args...)> {
    using Return = R;
    static constexpr std::size_t ArgumentCount = sizeof...(Args);
};

bool decodeStandardInstruction(const std::vector<std::int16_t> &program,
                               std::size_t programCounter,
                               std::size_t argumentCount,
                               bool conditional,
                               event::LegacyInstruction &instruction,
                               std::string &error) {
    const auto branchWords = conditional ? std::size_t{2} : std::size_t{0};
    const auto payloadWords = argumentCount + branchWords;
    if (programCounter >= program.size()
        || payloadWords > program.size() - programCounter - 1) {
        error = "truncated legacy event instruction";
        return false;
    }

    instruction.wordOffset = programCounter;
    instruction.opcode = program[programCounter];
    instruction.operands.assign(
        program.begin() + static_cast<std::ptrdiff_t>(programCounter + 1),
        program.begin() + static_cast<std::ptrdiff_t>(programCounter + 1
                                                       + argumentCount));
    instruction.conditional = conditional;
    instruction.nextWordOffset = programCounter + 1 + payloadWords;
    if (!conditional) {
        return true;
    }

    const auto trueAdvance = program[programCounter + 1 + argumentCount];
    const auto falseAdvance = program[programCounter + 2 + argumentCount];
    if (trueAdvance < 0 || falseAdvance < 0) {
        error = "negative legacy event branch advance";
        return false;
    }
    instruction.trueAdvance = static_cast<std::size_t>(trueAdvance);
    instruction.falseAdvance = static_cast<std::size_t>(falseAdvance);
    return true;
}

template <auto Handler, std::size_t... I>
event::LegacyHostResult invokeLegacyHandlerImpl(
        MapWithEvent *map,
        const event::LegacyInstruction &instruction,
        std::index_sequence<I...>) {
    using Traits = LegacyHandlerTraits<decltype(Handler)>;
    if (instruction.operands.size() != Traits::ArgumentCount) {
        return {event::VmStatus::Faulted, false,
                "legacy event operand count mismatch"};
    }
    if constexpr (std::is_same_v<typename Traits::Return, bool>) {
        const bool completed = Handler(map, instruction.operands[I]...);
        return {completed ? event::VmStatus::Running
                          : event::VmStatus::Waiting,
                false, {}};
    } else {
        const int branch = Handler(map, instruction.operands[I]...);
        if (branch < 0) {
            return {event::VmStatus::Waiting, false, {}};
        }
        return {event::VmStatus::Running, branch != 0, {}};
    }
}

template <auto Handler>
event::LegacyHostResult invokeLegacyHandler(
        MapWithEvent *map,
        const event::LegacyInstruction &instruction) {
    using Traits = LegacyHandlerTraits<decltype(Handler)>;
    return invokeLegacyHandlerImpl<Handler>(
        map, instruction,
        std::make_index_sequence<Traits::ArgumentCount>{});
}

#ifndef NDEBUG
void printInstruction(const event::LegacyInstruction &instruction) {
    std::fprintf(stdout, "%2d: {", instruction.opcode);
    for (const auto operand : instruction.operands) {
        std::fprintf(stdout, " %d", operand);
    }
    if (instruction.conditional) {
        std::fprintf(stdout, " %zu %zu", instruction.trueAdvance,
                     instruction.falseAdvance);
    }
    std::fprintf(stdout, " }\n");
    std::fflush(stdout);
}
#endif

#define HOJY_LEGACY_EVENT_HANDLERS(X) \
    X(0, closePopup) \
    X(1, doTalk) \
    X(2, addItem) \
    X(3, modifyEvent) \
    X(4, useItem) \
    X(5, askForWar) \
    X(8, changeExitMusic) \
    X(9, askForJoinTeam) \
    X(10, joinTeam) \
    X(11, wantSleep) \
    X(12, sleep) \
    X(13, makeBright) \
    X(14, makeDim) \
    X(15, die) \
    X(16, checkTeamMember) \
    X(17, changeLayer) \
    X(18, hasItem) \
    X(19, setPlayerPosition) \
    X(20, checkTeamFull) \
    X(21, leaveTeam) \
    X(22, emptyAllMP) \
    X(23, setAttrPoison) \
    X(24, die) \
    X(25, moveCamera) \
    X(26, modifyEventId) \
    X(27, animation) \
    X(28, checkIntegrity) \
    X(29, checkAttack) \
    X(30, walkPath) \
    X(31, checkMoney) \
    X(32, addItem2) \
    X(33, learnSkill) \
    X(34, addPotential) \
    X(35, setSkill) \
    X(36, checkSex) \
    X(37, addIntegrity) \
    X(38, modifySubMapLayerTex) \
    X(39, openSubMap) \
    X(40, forceDirection) \
    X(41, addItemToChar) \
    X(42, checkFemaleInTeam) \
    X(43, hasItem) \
    X(44, animation2) \
    X(45, addSpeed) \
    X(46, addMaxMP) \
    X(47, addAttack) \
    X(48, addMaxHP) \
    X(49, setMPType) \
    X(51, tutorialTalk) \
    X(52, showIntegrity) \
    X(53, showReputation) \
    X(54, openWorld) \
    X(55, checkEventID) \
    X(56, addReputation) \
    X(57, removeBarrier) \
    X(58, tournament) \
    X(59, disbandTeam) \
    X(60, checkSubMapTex) \
    X(61, checkAllStoryBooks) \
    X(62, goBackHome) \
    X(63, setSex) \
    X(64, openShop) \
    X(65, randomShop) \
    X(66, playMusic) \
    X(67, playSound)

}

bool MapWithEvent::decodeLegacy(
        const std::vector<std::int16_t> &program,
        std::size_t programCounter,
        event::LegacyInstruction &instruction,
        std::string &error) const {
    if (programCounter >= program.size()) {
        error = "legacy event program counter out of range";
        return false;
    }
    instruction = {};
    const auto opcode = program[programCounter];
    switch (opcode) {
    case -1:
    case 7:
        return decodeStandardInstruction(program, programCounter, 0, false,
                                         instruction, error);
    case 6: {
        constexpr std::size_t PayloadWords = 4;
        if (PayloadWords > program.size() - programCounter - 1) {
            error = "truncated legacy battle instruction";
            return false;
        }
        const auto trueAdvance = program[programCounter + 2];
        const auto falseAdvance = program[programCounter + 3];
        if (trueAdvance < 0 || falseAdvance < 0) {
            error = "negative legacy battle branch advance";
            return false;
        }
        instruction.opcode = opcode;
        instruction.wordOffset = programCounter;
        instruction.nextWordOffset = programCounter + 1 + PayloadWords;
        instruction.conditional = true;
        instruction.trueAdvance = static_cast<std::size_t>(trueAdvance);
        instruction.falseAdvance = static_cast<std::size_t>(falseAdvance);
        instruction.operands = {
            program[programCounter + 1],
            program[programCounter + 4],
        };
        return true;
    }
    case 50:
        if (programCounter + 1 >= program.size()) {
            error = "truncated legacy extended instruction";
            return false;
        }
        if (program[programCounter + 1] >= 128) {
            return decodeStandardInstruction(program, programCounter, 5, true,
                                             instruction, error);
        }
        return decodeStandardInstruction(program, programCounter, 7, false,
                                         instruction, error);
#define DecodeCase(Opcode, Handler) \
    case Opcode: { \
        using Traits = LegacyHandlerTraits<decltype(&MapWithEvent::Handler)>; \
        return decodeStandardInstruction( \
            program, programCounter, Traits::ArgumentCount, \
            std::is_same_v<typename Traits::Return, int>, instruction, error); \
    }
        HOJY_LEGACY_EVENT_HANDLERS(DecodeCase)
#undef DecodeCase
    default:
        instruction.opcode = opcode;
        instruction.wordOffset = programCounter;
        instruction.nextWordOffset = programCounter + 1;
        return true;
    }
}

event::LegacyHostResult MapWithEvent::executeLegacy(
        const event::LegacyInstruction &instruction,
        event::EventMemory &memory) {
#ifndef NDEBUG
    printInstruction(instruction);
#endif
    (void)memory;
    switch (instruction.opcode) {
    case -1:
    case 7:
        (void)exitEventList(this);
        return {event::VmStatus::Completed, false, {}};
    case 6: {
        if (instruction.operands.size() != 2) {
            return {event::VmStatus::Faulted, false,
                    "legacy battle operand count mismatch"};
        }
        const auto warId = instruction.operands[0];
        const auto getExpOnLose = instruction.operands[1] > 0;
        const auto eventSession = eventSessionToken();
        postOwnedSceneCommand(
            this,
            [eventSession, warId, getExpOnLose](
                MapWithEvent &owner, SceneCommandContext &context) {
            if (!owner.isCurrentEventSession(eventSession)) { return; }
            if (!context.enterWar(warId, getExpOnLose)
                && owner.isCurrentEventSession(eventSession)) {
                owner.continueEvents(false);
            }
        });
        return {event::VmStatus::Waiting, false, {}};
    }
    case 50:
        if (instruction.conditional) {
            return invokeLegacyHandler<&MapWithEvent::checkHas5Item>(
                this, instruction);
        }
        if (instruction.operands.size() != 7) {
            return {event::VmStatus::Faulted, false,
                    "extended event operand count mismatch"};
        } else {
            event::Instruction extended;
            extended.opcode = instruction.operands[0];
            for (std::size_t index = 0; index < extended.operands.size(); ++index) {
                extended.operands[index] = instruction.operands[index + 1];
            }
            const auto result = eventVm_.step(extended, *this);
            switch (result.status) {
            case event::VmStatus::Waiting:
                return {event::VmStatus::Waiting, false, {}};
            case event::VmStatus::Faulted:
                return {event::VmStatus::Faulted, false, result.error};
            case event::VmStatus::Running:
            case event::VmStatus::Completed:
                return {event::VmStatus::Running, false, {}};
            }
        }
        return {event::VmStatus::Faulted, false,
                "invalid extended event result"};
#define ExecuteCase(Opcode, Handler) \
    case Opcode: \
        return invokeLegacyHandler<&MapWithEvent::Handler>(this, instruction);
        HOJY_LEGACY_EVENT_HANDLERS(ExecuteCase)
#undef ExecuteCase
    default:
        return {event::VmStatus::Running, false, {}};
    }
}

void MapWithEvent::cleanupEvents() {
    const auto oldSession = eventSessionGeneration_;
    if (oldSession != 0) {
        postSceneCommand(this, [oldSession](SceneCommandContext &context) {
            context.clearEventPresentation({oldSession});
        });
    }
    ++eventSessionGeneration_;
    if (eventSessionGeneration_ == 0) { eventSessionGeneration_ = 1; }
    currEventPaused_ = false;
    currEventId_ = -1;
    currEventItem_ = -1;
    pendingSubEventWaiting_ = false;
    activeEventContinuationToken_ = 0;
    eventTimeoutDeadline_ = 0;
    eventTimeoutContinuationToken_ = 0;
    pendingSubEvents_.clear();
    moving_.clear();
    movingChar_ = false;
    animEventId_[0] = animEventId_[1] = animEventId_[2] = 0;
    animCurrTex_[0] = animCurrTex_[1] = animCurrTex_[2] = 0;
    animEndTex_[0] = animEndTex_[1] = animEndTex_[2] = 0;
    eventVm_.reset();
}

std::uint64_t MapWithEvent::beginEventContinuation() {
    ++nextEventContinuationToken_;
    if (nextEventContinuationToken_ == 0) {
        ++nextEventContinuationToken_;
    }
    activeEventContinuationToken_ = nextEventContinuationToken_;
    return activeEventContinuationToken_;
}

void MapWithEvent::applyEventInputContinuation(std::uint64_t token,
                                               std::int16_t value,
                                               bool writesMemory,
                                               std::int32_t destination) {
    if (token == 0 || token != activeEventContinuationToken_
        || !eventVm_.legacyWaiting()) {
        return;
    }
    activeEventContinuationToken_ = 0;
    if (writesMemory && !eventVm_.memory().writeWord(destination, value)) {
        std::fprintf(stderr, "failed to write event continuation result\n");
        cleanupEvents();
        return;
    }
    continueEvents(false);
}

void MapWithEvent::applyEventMenuContinuation(
        std::uint64_t token, std::int32_t destination,
        const EventMenuResult &result) {
    if (token == 0 || token != activeEventContinuationToken_
        || !eventVm_.legacyWaiting()) {
        return;
    }
    activeEventContinuationToken_ = 0;
    const auto value = result.accepted ? result.selection : 0;
    if (!eventVm_.memory().writeWord(destination, value)) {
        std::fprintf(stderr, "failed to write event menu result\n");
        cleanupEvents();
        return;
    }
    continueEvents(false);
}

void MapWithEvent::continueEvents(bool result) {
    if (!eventVm_.legacyActive() && pendingSubEvents_.empty()) {
        currEventPaused_ = false;
        pendingSubEventWaiting_ = false;
        return;
    }

    pendingSubEventWaiting_ = false;
    if (eventVm_.legacyWaiting() && !eventVm_.resumeLegacy(result)) {
        std::fprintf(stderr, "failed to resume legacy event program\n");
        cleanupEvents();
        return;
    }

    std::size_t executed = 0;
    while (executed < LegacyEventOperationBudget) {
        while (!pendingSubEvents_.empty()) {
            auto func = std::move(pendingSubEvents_.front());
            pendingSubEvents_.pop_front();
            if (!func()) {
                pendingSubEventWaiting_ = true;
                currEventPaused_ = true;
                return;
            }
        }
        if (!eventVm_.legacyActive()) {
            currEventPaused_ = false;
            currEventId_ = -1;
            return;
        }

        const auto vmResult = eventVm_.runLegacy(*this, 1);
        executed += vmResult.executed;
        if (vmResult.status == event::VmStatus::Waiting) {
            currEventPaused_ = true;
            return;
        }
        if (vmResult.status == event::VmStatus::Faulted) {
            std::fprintf(stderr, "legacy event fault: %s\n",
                         vmResult.error.c_str());
            cleanupEvents();
            return;
        }
        if (vmResult.status == event::VmStatus::Completed) {
            currEventPaused_ = false;
            currEventId_ = -1;
            return;
        }
    }
    currEventPaused_ = eventVm_.legacyActive() || !pendingSubEvents_.empty();
}

void MapWithEvent::runEvent(std::int16_t evt) {
    // A new VM program invalidates every presentation result belonging to the
    // previous program. Otherwise a stale menu/key callback could resume the
    // newly loaded program while it happens to be waiting.
    const auto eventId = currEventId_;
    cleanupEvents();
    currEventId_ = eventId;
    activeEventContinuationToken_ = 0;
    eventVm_.loadLegacy(::hojy::content::gEvent.event(evt));
    currEventPaused_ = eventVm_.legacyActive();
    pendingSubEventWaiting_ = false;
    if (!eventVm_.legacyDispatching()) {
        continueEvents(false);
    }
}

void MapWithEvent::onUseItem(std::int16_t itemId) {
    currEventItem_ = itemId;
    int x, y;
    if (!getFaceOffset(x, y)) {
        return;
    }
    checkEvent(1, x, y);
}

#undef HOJY_LEGACY_EVENT_HANDLERS

}
