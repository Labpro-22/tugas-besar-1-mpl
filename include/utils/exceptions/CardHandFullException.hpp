#pragma once

#include "NimonspoliException.hpp"

class SkillCard;

class CardHandFullException : public NimonspoliException {
private:
    std::string playerUsername;
    SkillCard* newCard;

public:
    CardHandFullException(const std::string& username, SkillCard* newCard)
        : NimonspoliException(username + " sudah memiliki 3 kartu"),
          playerUsername(username),
          newCard(newCard) {}

    const std::string& getPlayerUsername() const { return playerUsername; }
    SkillCard* getNewCard() const { return newCard; }
};
