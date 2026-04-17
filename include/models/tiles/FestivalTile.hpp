#pragma once

#include "ActionTile.hpp"

class FestivalTile : public ActionTile {
public:
    FestivalTile();
    FestivalTile(int index, const std::string& code, const std::string& name);

    void onLanded(Player& player, GameContext& gameContext) override;
    std::string getDisplayLabel() const override;
};
