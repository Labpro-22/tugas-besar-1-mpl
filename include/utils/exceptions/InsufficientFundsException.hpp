#pragma once

#include "NimonspoliException.hpp"
#include <string>

class InsufficientFundsException : public NimonspoliException {
    private:
        int required;
        int available;
    public:
        InsufficientFundsException(int required, int available)
            : NimonspoliException("Membutuhkan M" + std::to_string(required) + ", tersedia M" + std::to_string(available)),
                required(required), available(available) {}
        int getRequired() const {return required;}
        int getAvailable() const {return available;}
};