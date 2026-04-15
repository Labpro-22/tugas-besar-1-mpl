#pragma once

#include "NimonspoliException.hpp"
#include <string>

class InvalidPropertyException : public NimonspoliException {
private:
    std::string invalidCode;
    std::string reason;

public:
    InvalidPropertyException(const std::string& code, const std::string& reason)
        : NimonspoliException("Properti '" + code + "' tidak valid: " + reason),
            invalidCode(code), reason(reason) {}

    const std::string& getInvalidCode() const {return invalidCode;}
    const std::string& getReason() const {return reason;}
};