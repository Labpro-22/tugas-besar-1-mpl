#pragma once

#include <exception>
#include <string>

class TransactionLogger;

class GameIO {
public:
    virtual ~GameIO() = default;

    virtual int promptInt(const std::string& prompt) = 0;
    virtual int promptIntInRange(const std::string& prompt, int minValue, int maxValue) = 0;
    virtual void showMessage(const std::string& message) = 0;
    virtual void showError(
        const std::exception& exception,
        TransactionLogger* logger = nullptr,
        int turn = 0,
        const std::string& username = "SYSTEM"
    ) = 0;
};
