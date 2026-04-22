#pragma once

#include <exception>
#include <string>

class SkillCard;

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

class InsufficientFundsException : public NimonspoliException {
private:
    int required;
    int available;

public:
    InsufficientFundsException(int required, int available)
        : NimonspoliException(
              "Membutuhkan M" + std::to_string(required)
                  + ", tersedia M" + std::to_string(available)),
          required(required),
          available(available) {}

    int getRequired() const {
        return required;
    }

    int getAvailable() const {
        return available;
    }
};

class CardHandFullException : public NimonspoliException {
private:
    std::string playerUsername;
    SkillCard* newCard;

public:
    CardHandFullException(const std::string& username, SkillCard* newCard)
        : NimonspoliException(username + " sudah memiliki 3 kartu"),
          playerUsername(username),
          newCard(newCard) {}

    const std::string& getPlayerUsername() const {
        return playerUsername;
    }

    SkillCard* getNewCard() const {
        return newCard;
    }
};

class InvalidCommandException : public NimonspoliException {
private:
    std::string commandKeyword;
    std::string reason;

public:
    InvalidCommandException(const std::string& keyword, const std::string& reason)
        : NimonspoliException("Perintah '" + keyword + "': " + reason),
          commandKeyword(keyword),
          reason(reason) {}

    const std::string& getCommandKeyword() const {
        return commandKeyword;
    }

    const std::string& getReason() const {
        return reason;
    }
};

class InvalidPropertyException : public NimonspoliException {
private:
    std::string invalidCode;
    std::string reason;

public:
    InvalidPropertyException(const std::string& code, const std::string& reason)
        : NimonspoliException("Properti '" + code + "' tidak valid: " + reason),
          invalidCode(code),
          reason(reason) {}

    const std::string& getInvalidCode() const {
        return invalidCode;
    }

    const std::string& getReason() const {
        return reason;
    }
};
