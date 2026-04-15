#pragma once

#include "NimonspoliException.hpp"
#include <string>
 
class InvalidCommandException : public NimonspoliException {
private:
    std::string commandKeyword;
    std::string reason;
 
public:
    InvalidCommandException(const std::string& keyword, const std::string& reason)
        : NimonspoliException("Perintah '" + keyword + "': " + reason),
            commandKeyword(keyword), reason(reason) {}
 
    const std::string& getCommandKeyword() const {return commandKeyword;}
    const std::string& getReason() const {return reason;}
};