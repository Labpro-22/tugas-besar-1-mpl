#pragma once

#include "ActionTile.hpp"

class GoToJailTile : public ActionTile {
public:
    GoToJailTile();
    GoToJailTile(int index, const std::string& code, const std::string& name);

    void onLanded(Player& player, GameContext& gameContext) override;
};
