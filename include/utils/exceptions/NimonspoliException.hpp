#pragma once

#include <exception>
#include <string>

class NimonspoliException : public std::exception {
protected:
    std::string message;

public:
    explicit NimonspoliException(const std::string& msg) : message(msg) {}
    virtual ~NimonspoliException() = default;

    const char* what() const noexcept override {
        return message.c_str();
    }

    std::string getMessage() const {
        return message;
    }
};
