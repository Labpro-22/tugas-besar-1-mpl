#include "models/Player.hpp"

Player::Player()
    : username(""),
      balance(0),
      position(0),
      status(PlayerStatus::ACTIVE),
      jailTurns(0),
      consecutiveDoubles(0),
      shieldActive(false),
      discountPercent(0),
      usedSkillThisTurn(false),
      rolledThisTurn(false) {}

Player::Player(const std::string& username, int initialBalance)
    : username(username),
      balance(initialBalance),
      position(0),
      status(PlayerStatus::ACTIVE),
      jailTurns(0),
      consecutiveDoubles(0),
      shieldActive(false),
      discountPercent(0),
      usedSkillThisTurn(false),
      rolledThisTurn(false) {}

const std::string& Player::getUsername() const {
    return username;
}
