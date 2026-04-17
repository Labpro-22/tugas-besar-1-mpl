#pragma once

#include <string>

class GameContext;
class Player;

class ActionCard {
private:
    std::string text;

public:
    ActionCard();
    explicit ActionCard(const std::string& text);
    virtual ~ActionCard() = default;

    const std::string& getText() const;
    virtual void execute(Player& player, GameContext& gameContext) = 0;
};
