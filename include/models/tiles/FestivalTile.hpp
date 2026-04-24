#pragma once

#include "ActionTile.hpp"
#include <vector>

class StreetTile;

class FestivalTile : public ActionTile {
public:
    FestivalTile();
    FestivalTile(int index, const std::string& code, const std::string& name);

    void onLanded(Player& player, GameContext& gameContext) override;
    
    // Getters untuk data inquiry (untuk GameEngine handling output & input)
    void getPlayerStreets(const Player& player, std::vector<StreetTile*>& outStreets) const;
    void applyFestivalEffect(StreetTile* selectedStreet, Player& player, GameContext& gameContext) const;
};
