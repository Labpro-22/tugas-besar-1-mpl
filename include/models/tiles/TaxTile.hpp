#pragma once

#include "ActionTile.hpp"

class TaxTile : public ActionTile {
private:
    TaxType taxType;
    int flatAmount;
    int percentage;

    int calculateWealth(const Player& player) const;

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
    std::string getDisplayLabel() const override;
    TaxType getTaxType() const;
    int getFlatAmount() const;
    int getPercentage() const;
};
