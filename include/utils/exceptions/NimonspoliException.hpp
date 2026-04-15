#pragma once

#include <exception>
#include <string>

class NimonspoliExeption :: public std::exception {
    protected:
        std::string message;
    public:
        explicit NimonspoliExeption(const std::string& msg) : message(msg) {}
        virtual ~NimonspoliExeption() = default;

        const char* what() const noexcept override {
            return message.c_str();
        }

        std::string getMessage() const {
            return message;
        }
}