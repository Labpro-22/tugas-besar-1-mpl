#pragma once

#include "ActionCard.hpp"

class BirthdayCard : public ActionCard {
private:
    int amount;

public:
    BirthdayCard();
    explicit BirthdayCard(int amount);

    int getAmount() const;
    void execute(Player& player, GameContext& gameContext) override;
};
