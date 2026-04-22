#include "models/state/PlayerState.hpp"

PlayerState::PlayerState()
    : username(""),
      balance(0),
      position(0),
      positionCode("GO"),
      status(PlayerStatus::ACTIVE),
      jailTurns(0),
      cardHand() {}


PlayerState::PlayerState(
    const std::string& username,
    int balance,
    int position,
    const std::string& positionCode,
    PlayerStatus status,
    int jailTurns,
    const std::vector<std::string>& cardHand
)
    : username(username),
      balance(balance),
      position(position),
      positionCode(positionCode),
      status(status),
      jailTurns(jailTurns),
      cardHand(cardHand) {}


const std::string& PlayerState::getUsername() const {
    return username;
}

int PlayerState::getBalance() const {
    return balance;
}

int PlayerState::getPosition() const {
    return position;
}

const std::string& PlayerState::getPositionCode() const {
    return positionCode;
}

PlayerStatus PlayerState::getStatus() const {
    return status;
}

int PlayerState::getJailTurns() const {
    return jailTurns;
}

std::vector<std::string>& PlayerState::getCardHand() {
    return cardHand;
}

const std::vector<std::string>& PlayerState::getCardHand() const {
    return cardHand;
}
