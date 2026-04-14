#pragma once

class MiscConfig {
private:
    int maxTurn;
    int initialBalance;

public:
    MiscConfig();
    MiscConfig(int maxTurn, int initialBalance);

    int getMaxTurn() const;
    int getInitialBalance() const;
};
