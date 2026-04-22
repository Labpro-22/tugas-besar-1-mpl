#pragma once

#include <exception>
#include <iostream>
#include <string>

#include "utils/TransactionLogger.hpp"
#include "utils/exceptions/NimonspoliException.hpp"

class ExceptionHandler {
private:
    static bool startsWith(const std::string& text, const std::string& prefix) {
        return text.rfind(prefix, 0) == 0;
    }

    static std::string extractQuotedValue(
        const std::string& text,
        const std::string& prefix
    ) {
        if (!startsWith(text, prefix)) {
            return "";
        }

        const std::size_t begin = prefix.size();
        const std::size_t end = text.find('\'', begin);

        if (end == std::string::npos) {
            return "";
        }

        return text.substr(begin, end - begin);
    }

public:
    static std::string formatMessage(const std::exception& exception) {
        const NimonspoliException* gameException =
            dynamic_cast<const NimonspoliException*>(&exception);
        if (gameException != nullptr) {
            return gameException->getMessage();
        }

        const std::string rawMessage = exception.what();

        if (startsWith(rawMessage, "SaveManager: cannot open '")) {
            const std::string filename =
                extractQuotedValue(rawMessage, "SaveManager: cannot open '");

            if (rawMessage.find("for reading") != std::string::npos) {
                return "File \"" + filename + "\" tidak ditemukan.";
            }

            if (rawMessage.find("for writing") != std::string::npos) {
                return "Gagal menyimpan file! Pastikan direktori dapat ditulis.";
            }
        }

        if (startsWith(rawMessage, "SaveManager:")) {
            return "Gagal memuat file! File rusak atau format tidak dikenali.";
        }

        if (startsWith(rawMessage, "ConfigLoader: cannot open '")) {
            const std::string filename =
                extractQuotedValue(rawMessage, "ConfigLoader: cannot open '");
            return "Gagal membuka file konfigurasi: " + filename;
        }

        if (startsWith(rawMessage, "ConfigLoader:")) {
            return "Gagal membaca file konfigurasi. Pastikan format file valid.";
        }

        if (startsWith(rawMessage, "CardDeck:")) {
            return "Deck kartu kosong atau tidak dapat digunakan.";
        }

        if (rawMessage.find("Board belum terbangun") != std::string::npos) {
            return "Papan permainan belum siap. Pastikan game berhasil diinisialisasi.";
        }

        if (rawMessage.find("Konfigurasi petak") != std::string::npos ||
            rawMessage.find("Konfigurasi properti") != std::string::npos ||
            rawMessage.find("ConfigData") != std::string::npos) {
            return "Konfigurasi permainan tidak valid: " + rawMessage;
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
