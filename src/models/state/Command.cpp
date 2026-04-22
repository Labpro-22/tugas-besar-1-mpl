#include "models/state/Command.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>

namespace {
    std::string normalizeKeyword(const std::string& keyword) {
        std::string normalized = keyword;
        std::transform(
            normalized.begin(),
            normalized.end(),
            normalized.begin(),
            [](unsigned char c) { return static_cast<char>(std::toupper(c)); }
        );
        return normalized;
    }

    bool isKnownCommand(const std::string& keyword) {
        return keyword == "CETAK_PAPAN" ||
               keyword == "LEMPAR_DADU" ||
               keyword == "ATUR_DADU" ||
               keyword == "CETAK_AKTA" ||
               keyword == "CETAK_PROPERTI" ||
               keyword == "GADAI" ||
               keyword == "TEBUS" ||
               keyword == "BANGUN" ||
               keyword == "SIMPAN" ||
               keyword == "MUAT" ||
               keyword == "CETAK_LOG" ||
               keyword == "BAYAR_DENDA" ||
               keyword == "GUNAKAN_KEMAMPUAN" ||
               keyword == "HELP" ||
               keyword == "KELUAR";
    }

    bool isValidArgCount(const std::string& keyword, int argCount) {
        if (keyword == "CETAK_PAPAN") {
            return argCount == 0;
        }

        if (keyword == "LEMPAR_DADU") {
            return argCount == 0;
        }

        if (keyword == "ATUR_DADU") {
            return argCount == 2;
        }

        if (keyword == "CETAK_AKTA") {
            return argCount == 1;
        }

        if (keyword == "CETAK_PROPERTI") {
            return argCount == 0;
        }

        if (keyword == "GADAI") {
            return argCount == 0;
        }

        if (keyword == "TEBUS") {
            return argCount == 0;
        }

        if (keyword == "BANGUN") {
            return argCount == 0;
        } 

        if (keyword == "SIMPAN") {
            return argCount == 1;
        }

        if (keyword == "MUAT") {
            return argCount == 1;
        }

        if (keyword == "CETAK_LOG") {
            return argCount == 0 || argCount == 1;
        }

        if (keyword == "BAYAR_DENDA") {
            return argCount == 0;
        }

        if (keyword == "GUNAKAN_KEMAMPUAN") {
            return argCount == 0;
        }

        if (keyword == "HELP" || keyword == "KELUAR") {
            return argCount == 0;
        }

        return false;
    }
}

Command::Command() : keyword(""), args() {}

Command::Command(const std::string& keyword, const std::vector<std::string>& args)
    : keyword(normalizeKeyword(keyword)), args(args) {}

const std::string& Command::getKeyword() const {
    return keyword;
}

std::vector<std::string>& Command::getArgs() {
    return args;
}

const std::vector<std::string>& Command::getArgs() const {
    return args;
}

std::string Command::getArg(int index) const {
    if (index < 0 || index >= static_cast<int>(args.size())) {
        throw std::out_of_range("Commad::getArg index di luar batas.");
    }

    return args[static_cast<std::size_t>(index)];
}

int Command::getArgCount() const {
    return static_cast<int>(args.size());
}

bool Command::isValid() const {
    if (keyword.empty()) {
        return false;
    }

    if (!isKnownCommand(keyword)) {
        return false;
    }

    return isValidArgCount(keyword, getArgCount());
}
