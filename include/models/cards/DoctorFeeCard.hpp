#pragma once

#include "ActionCard.hpp"

class DoctorFeeCard : public ActionCard {
private:
    int amount;

public:
    DoctorFeeCard();
    explicit DoctorFeeCard(int amount);

    void execute(Player& player, GameContext& gameContext) override;
};
