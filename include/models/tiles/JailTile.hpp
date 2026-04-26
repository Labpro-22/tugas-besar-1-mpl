#pragma once

#include "ActionTile.hpp"

class JailTile : public ActionTile {
private:
    int jailFine;

public:
    JailTile();
    JailTile(int index, const std::string& code, const std::string& name, int jailFine);

    void onLanded(Player& player, GameContext& gameContext) override;
    void applyJailStatus(Player& player) const override;
    void processJailTurn(Player& player, GameContext& gameContext) const;
    int getJailFine() const override;
};
