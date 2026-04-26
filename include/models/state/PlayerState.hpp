#pragma once

#include <string>
#include <vector>

#include "models/Enums.hpp"

class PlayerState {
private:
    std::string username;
    int balance;
    int position;
    std::string positionCode;
    PlayerStatus status;
    int jailTurns;
    std::vector<std::string> cardHand;

public:
    PlayerState();
    PlayerState(
        const std::string& username,
        int balance,
        int position,
        const std::string& positionCode,
        PlayerStatus status,
        int jailTurns,
        const std::vector<std::string>& cardHand
    );

    const std::string& getUsername() const;
    int getBalance() const;
    int getPosition() const;
    const std::string& getPositionCode() const;
    PlayerStatus getStatus() const;
    int getJailTurns() const;
    std::vector<std::string>& getCardHand();
    const std::vector<std::string>& getCardHand() const;
};

