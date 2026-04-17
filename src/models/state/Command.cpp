#include "models/state/Command.hpp"

Command::Command() : keyword(""), args() {}

Command::Command(const std::string& keyword, const std::vector<std::string>& args)
    : keyword(keyword), args(args) {}

const std::string& Command::getKeyword() const {
    return keyword;
}
