#pragma once

#include "models/tiles/PropertyTile.hpp"
#include "models/Player.hpp"

class PropertyCardRenderer {
public:
    void renderDeed(PropertyTile* property);
    void renderPlayerProperties(Player& player);
    void renderAuctionUI(PropertyTile* property, int currentBid, Player* bidder);
};