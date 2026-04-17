#pragma once

#include <string>

class GameContext;
class Player;

class SkillCard {
private:
    int value;
    int remainingDuration;

public:
    SkillCard();
    SkillCard(int value, int remainingDuration);
    virtual ~SkillCard() = default;

    int getValue() const;
    int getRemainingDuration() const;
    void setRemainingDuration(int remainingDuration);

    virtual std::string getTypeName() const = 0;
    virtual void use(Player& player, GameContext& gameContext) = 0;
};
