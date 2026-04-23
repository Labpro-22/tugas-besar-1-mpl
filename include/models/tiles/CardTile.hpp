#pragma once

#include "ActionTile.hpp"

class CardTile : public ActionTile {
private:
    CardType cardType;

public:
    CardTile();
    CardTile(int index, const std::string& code, const std::string& name, CardType cardType);

    void onLanded(Player& player, GameContext& gameContext) override;
    CardType getCardType() const;
};
