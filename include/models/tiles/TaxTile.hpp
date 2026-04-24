#pragma once

#include "models/Enums.hpp"
#include "ActionTile.hpp"

class TaxTile : public ActionTile {
private:
    TaxType taxType;
    int flatAmount;
    int percentage;

    int calculateWealth(const Player& player) const;
    void applyTax(Player& player, GameContext& gameContext, int amountToPay, int choice) const;

public:
    TaxTile();
    TaxTile(
        int index,
        const std::string& code,
        const std::string& name,
        TaxType taxType,
        int flatAmount,
        int percentage
    );

    void onLanded(Player& player, GameContext& gameContext) override;
    int calculateTaxAmount(const Player& player, int choice = 1) const;
};
