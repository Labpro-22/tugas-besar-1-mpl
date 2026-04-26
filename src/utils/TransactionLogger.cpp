#include "utils/TransactionLogger.hpp"

void TransactionLogger::log(int turn, const std::string& username, const std::string& actionType, const std::string& detail) {
	entries.emplace_back(turn, username, actionType, detail);
}

const std::vector<LogEntry>& TransactionLogger::getAll() const {
	return entries;
}

std::vector<LogEntry> TransactionLogger::getLastN(int n) const {
	if (n <= 0 || entries.empty()) {
		return {};
	}

	int size = entries.size();
    int start;
	if (n >= size){
        start = 0;
    } 
    else {
        start = (size - n);
    }
    
	return std::vector<LogEntry>(entries.begin() + start, entries.end());
}

void TransactionLogger::clear() {
	entries.clear();
}

void TransactionLogger::importEntries(const std::vector<LogEntry>& savedEntries) {
	entries = savedEntries;
}
