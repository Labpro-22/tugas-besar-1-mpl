#include "models/state/LogEntry.hpp"

LogEntry::LogEntry() : turn(0), username(""), actionType(""), detail("") {}

LogEntry::LogEntry(
    int turn,
    const std::string& username,
    const std::string& actionType,
    const std::string& detail
) : turn(turn), username(username), actionType(actionType), detail(detail) {}

int LogEntry::getTurn() const {
    return turn;
}

const std::string& LogEntry::getUsername() const {
    return username;
}

const std::string& LogEntry::getActionType() const {
    return actionType;
}

const std::string& LogEntry::getDetail() const {
    return detail;
}
