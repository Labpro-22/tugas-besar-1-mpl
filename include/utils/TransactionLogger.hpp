#pragma once

#include <vector>
#include <string>

#include "models/state/LogEntry.hpp"

class TransactionLogger {
private:
    std::vector<LogEntry> entries;
public:
    TransactionLogger() = default;

    void log(int turn, const std::string& username, const std::string& actionType, const std::string& detail);
    const std::vector<LogEntry>& getAll() const;
    std::vector<LogEntry> getLastN(int n) const;
    void clear();
    void importEntries(const std::vector<LogEntry>& savedEntries);
};
