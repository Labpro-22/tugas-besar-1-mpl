#include "core/CommandResult.hpp"

CommandResult::CommandResult(bool exitRequested, bool turnFinished)
    : exitRequested(exitRequested), turnFinished(turnFinished) {}

bool CommandResult::isExitRequested() const {
    return exitRequested;
}

bool CommandResult::isTurnFinished() const {
    return turnFinished;
}
