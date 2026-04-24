#pragma once

#include <string>

class LogEntry {
private:
    int turn;
    std::string username;
    std::string actionType;
    std::string detail;

public:
    LogEntry();
    LogEntry(
        int turn,
        const std::string& username,
        const std::string& actionType,
        const std::string& detail
    );

    int getTurn() const;
    const std::string& getUsername() const;
    const std::string& getActionType() const;
    const std::string& getDetail() const;
};
