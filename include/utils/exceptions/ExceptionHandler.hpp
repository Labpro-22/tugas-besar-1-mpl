#pragma once

#include <exception>
#include <iostream>
#include <string>

#include "utils/TransactionLogger.hpp"
#include "utils/exceptions/NimonspoliException.hpp"

class ExceptionHandler {
public:
    static std::string formatMessage(const std::exception& exception) {
        const NimonspoliException* gameException =
            dynamic_cast<const NimonspoliException*>(&exception);
        if (gameException != nullptr) {
            return gameException->getMessage();
        }

        const std::string rawMessage = exception.what();

        if (dynamic_cast<const std::out_of_range*>(&exception) != nullptr) {
            return rawMessage.empty()
                ? "Nilai berada di luar rentang yang diizinkan."
                : rawMessage;
        }

        if (dynamic_cast<const std::runtime_error*>(&exception) != nullptr &&
            rawMessage.find("Input dihentikan") != std::string::npos) {
            return rawMessage;
        }

        if (!rawMessage.empty()) {
            return rawMessage;
        }

        return "Terjadi error yang tidak diketahui.";
    }

    static void logException(
        const std::string& message,
        TransactionLogger* logger = nullptr,
        int turn = 0,
        const std::string& username = "SYSTEM",
        const std::string& actionType = "ERROR"
    ) {
        if (logger == nullptr) {
            return;
        }

        logger->log(turn, username, actionType, message);
    }

    static void handle(
        const std::exception& exception,
        std::ostream& output = std::cerr,
        TransactionLogger* logger = nullptr,
        int turn = 0,
        const std::string& username = "SYSTEM",
        const std::string& actionType = "ERROR"
    ) {
        const std::string message = formatMessage(exception);
        output << message << std::endl;
        logException(message, logger, turn, username, actionType);
    }

    static void handleUnknown(
        std::ostream& output = std::cerr,
        TransactionLogger* logger = nullptr,
        int turn = 0,
        const std::string& username = "SYSTEM",
        const std::string& actionType = "ERROR"
    ) {
        const std::string message = "Terjadi error yang tidak diketahui.";
        output << message << std::endl;
        logException(message, logger, turn, username, actionType);
    }
};
