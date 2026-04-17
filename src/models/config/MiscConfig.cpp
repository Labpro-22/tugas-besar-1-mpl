#include "models/config/MiscConfig.hpp"

MiscConfig::MiscConfig() : maxTurn(0), initialBalance(0) {}

MiscConfig::MiscConfig(int maxTurn, int initialBalance)
    : maxTurn(maxTurn), initialBalance(initialBalance) {}

int MiscConfig::getMaxTurn() const {
    return maxTurn;
}

int MiscConfig::getInitialBalance() const {
    return initialBalance;
}
