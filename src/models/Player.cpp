#include "models/Player.hpp"

#include <algorithm>
#include <stdexcept>

#include "utils/exceptions/NimonspoliException.hpp"
#include "models/tiles/PropertyTile.hpp"
#include "models/tiles/StreetTile.hpp"
#include "models/cards/SkillCard.hpp"

namespace {
    const int DEFAULT_BOARD_TILE_COUNT = 40;
}

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
      rolledThisTurn(false),
      actionTakenThisTurn(false) {}

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
      rolledThisTurn(false),
      actionTakenThisTurn(false) {}

// OPERATOR OVERLOADING

Player& Player::operator+=(int amount) {
    if (amount < 0) {
        return (*this) -= -amount;
    }

    balance += amount;
    return *this;
}
    
Player& Player::operator-=(int amount) {
    if (amount < 0) {
        return (*this) += -amount;
    }

    if (balance < amount) {
        throw InsufficientFundsException(amount, balance);
    }
    
    balance -= amount;
    return *this;
}
    
bool Player::operator<(const Player& other) const {
    return getTotalWealth() < other.getTotalWealth();
}

bool Player::operator>(const Player& other) const {
    return getTotalWealth() > other.getTotalWealth() ;
}

// Navigasi Move
bool Player::moveTo(int newIndex) {
    bool passedGo = false;
    if (newIndex >= DEFAULT_BOARD_TILE_COUNT) {
        passedGo = true;
        newIndex = newIndex % DEFAULT_BOARD_TILE_COUNT;
    } else if (newIndex < 0) {
        newIndex = ((newIndex % DEFAULT_BOARD_TILE_COUNT) + DEFAULT_BOARD_TILE_COUNT) % DEFAULT_BOARD_TILE_COUNT;
    } else if (newIndex == 0 && position != 0) {
        passedGo = true;
    }

    position = newIndex;
    return passedGo;
}

// Manajemen Properti
void Player::addProperty(PropertyTile* tile) {
    if (tile == nullptr) return;
    properties.push_back(tile);
}
    
void Player::removeProperty(PropertyTile* tile) {
    auto it = std::find(properties.begin(), properties.end(), tile);
    if (it != properties.end()) {
        properties.erase(it);
    }
}

// Manajemen Kartu
void Player::addCard(SkillCard* card) {
    if (hand.size() >= 3) {
        throw CardHandFullException(username, card);
    }
    hand.push_back(card);
}

void Player::removeCard(SkillCard* card) {
    auto it = std::find(hand.begin(), hand.end(), card);
    if (it != hand.end()) {
        hand.erase(it);
    }
}

void Player::clearHand() {
    hand.clear();
}

// Query/Calculation
int Player::getTotalWealth() const {
    int total = balance;
    for (const PropertyTile* prop : properties) {
        if (prop == nullptr) {
            continue;
        }

        total += prop->getBuyPrice();
        if (prop->getPropertyType() == PropertyType::STREET) {
            int level = prop->getBuildingLevel();
            if (level > 0 && level <= 4) {
                total += level * prop->getHouseCost();
            } else if (level == 5) {
                total += (4 * prop->getHouseCost()) + prop->getHotelCost();
            }
        }
    }
    return total;
}

int Player::getLiquidationMax() const {
    int total = balance;
    for (const PropertyTile* prop : properties) {
        if (prop == nullptr || prop->getStatus() == PropertyStatus::MORTGAGED) {
            continue;
        }

        total += std::max(prop->getSellValueToBank(), prop->getMortgageValue());
    }
    return total;
}

int Player::getDiscountedAmount(int amount) const {
    if (amount <= 0 || discountPercent <= 0) {
        return amount;
    }

    int discount = (amount * discountPercent) / 100;
    return std::max(0, amount - discount);
}

int Player::consumeDiscountedAmount(int amount) {
    // DiscountCard berlaku selama giliran aktif, jadi nilainya baru di-reset
    // saat turn berikutnya dimulai melalui resetTurnState().
    return getDiscountedAmount(amount);
}

bool Player::canAfford(int amount) const {
    return balance >= amount;
}

bool Player::isActive() const {
    return status == PlayerStatus::ACTIVE;
}

bool Player::isJailed() const {
    return status == PlayerStatus::JAILED;
}

bool Player::isBankrupt() const {
    return status == PlayerStatus::BANKRUPT;
}

void Player::clearTemporarySkillEffects() {
    shieldActive = false;
    discountPercent = 0;
}

void Player::resetTurnState() {
    clearTemporarySkillEffects();
    usedSkillThisTurn = false;
    rolledThisTurn = false;
    actionTakenThisTurn = false;
}

// Getters
const std::string& Player::getUsername() const {
    return username;
}

int Player::getBalance() const {
    return balance;
}

int Player::getPosition() const {
    return position;
}
    
PlayerStatus Player::getStatus() const {
    return status;
}

const std::vector<PropertyTile*>& Player::getProperties() const {
    return properties;
}

const std::vector<SkillCard*>& Player::getHand() const {
    return hand;
}

int Player::getJailTurns() const {
    return jailTurns;
}
    
int Player::getConsecutiveDoubles() const {
    return consecutiveDoubles;
}
    
bool Player::isShieldActive() const {
    return shieldActive;
}

int Player::getDiscountPercent() const {
    return discountPercent;
}
    
bool Player::hasUsedSkillThisTurn() const {
    return usedSkillThisTurn;
}

bool Player::hasRolledThisTurn() const {
    return rolledThisTurn;
}

bool Player::hasTakenActionThisTurn() const {
    return actionTakenThisTurn;
}

// Setters
void Player::setBalance(int balance) {
    this->balance = balance;
}

void Player::setPosition(int position) {
    this->position = position;
}

void Player::setStatus(PlayerStatus status) {
    this->status = status;
}

void Player::setJailTurns(int jailTurns) {
    this->jailTurns = jailTurns;
}

void Player::setConsecutiveDoubles(int consecutiveDoubles) {
    this->consecutiveDoubles = consecutiveDoubles;
}

void Player::setShieldActive(bool shieldActive) {
    this->shieldActive = shieldActive;
}

void Player::setDiscountPercent(int discountPercent) {
    this->discountPercent = discountPercent;
}

void Player::setUsedSkillThisTurn(bool usedSkillThisTurn) {
    this->usedSkillThisTurn = usedSkillThisTurn;
}
    
void Player::setHasRolledThisTurn(bool hasRolledThisTurn) {
    this->rolledThisTurn = hasRolledThisTurn;
}

void Player::setActionTakenThisTurn(bool actionTakenThisTurn) {
    this->actionTakenThisTurn = actionTakenThisTurn;
}
