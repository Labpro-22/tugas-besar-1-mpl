#pragma once

#include <string>

class GameContext;
class Player;

class SkillCard {
private:
    int value;

public:
    SkillCard();
    explicit SkillCard(int value);
    virtual ~SkillCard() = default;

    int getValue() const;

    virtual bool canUseWhileJailed() const;
    virtual std::string getTypeName() const = 0;
    virtual void use(Player& player, GameContext& gameContext) = 0;
};
