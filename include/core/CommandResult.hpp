#pragma once

class CommandResult {
private:
    bool exitRequested;
    bool turnFinished;

public:
    CommandResult(bool exitRequested = false, bool turnFinished = false);

    bool isExitRequested() const;
    bool isTurnFinished() const;
};
