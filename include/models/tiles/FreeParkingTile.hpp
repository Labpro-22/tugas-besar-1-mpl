#pragma once

#include "ActionTile.hpp"

class FreeParkingTile : public ActionTile {
public:
    FreeParkingTile();
    FreeParkingTile(int index, const std::string& code, const std::string& name);

    void onLanded(Player& player, GameContext& gameContext) override;
    std::string getDisplayLabel() const override;
};
