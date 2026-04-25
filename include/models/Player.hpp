#pragma once

#include <string>
#include <vector>

#include "models/Enums.hpp"

class PropertyTile;
class StreetTile;
class SkillCard;

class Player {
private:
    std::string username;
    int balance;
    int position;
    PlayerStatus status;
    std::vector<PropertyTile*> properties;
    std::vector<SkillCard*> hand;
    int jailTurns;
    int consecutiveDoubles;
    bool shieldActive;
    int discountPercent;
    bool usedSkillThisTurn;
    bool rolledThisTurn;
    bool movementDiceRolledThisTurn;
    bool actionTakenThisTurn;

public:
    Player();
    Player(const std::string& username, int initialBalance);

    Player& operator+=(int amount);
    Player& operator-=(int amount);
    bool operator<(const Player& other) const;
    bool operator>(const Player& other) const;

    bool moveTo(int newIndex);
    void addProperty(PropertyTile* tile);
    void removeProperty(PropertyTile* tile);

    void addCard(SkillCard* card);
    void removeCard(SkillCard* card);
    void clearHand();

    int getTotalWealth() const;
    int getLiquidationMax() const;
    int getDiscountedAmount(int amount) const;
    int consumeDiscountedAmount(int amount);
    bool canAfford(int amount) const;
    bool isActive() const;
    bool isJailed() const;
    bool isBankrupt() const;
    void clearTemporarySkillEffects();
    void resetTurnState();

    const std::string& getUsername() const;
    int getBalance() const;
    int getPosition() const;
    PlayerStatus getStatus() const;
    const std::vector<PropertyTile*>& getProperties() const;
    const std::vector<SkillCard*>& getHand() const;
    int getJailTurns() const;
    int getConsecutiveDoubles() const;
    bool isShieldActive() const;
    int getDiscountPercent() const;
    bool hasUsedSkillThisTurn() const;
    bool hasRolledThisTurn() const;
    bool hasRolledMovementDiceThisTurn() const;
    bool hasTakenActionThisTurn() const;

    void setBalance(int balance);
    void setPosition(int position);
    void setStatus(PlayerStatus status);
    void setJailTurns(int jailTurns);
    void setConsecutiveDoubles(int consecutiveDoubles);
    void setShieldActive(bool shieldActive);
    void setDiscountPercent(int discountPercent);
    void setUsedSkillThisTurn(bool usedSkillThisTurn);
    void setHasRolledThisTurn(bool hasRolledThisTurn);
    void setHasRolledMovementDiceThisTurn(bool hasRolledMovementDiceThisTurn);
    void setActionTakenThisTurn(bool actionTakenThisTurn);
};
