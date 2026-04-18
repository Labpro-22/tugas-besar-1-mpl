#pragma once

#include "models/tiles/PropertyTile.hpp"
#include "models/Player.hpp"

class PropertyCardRenderer {
public:
    void renderDeed(const PropertyTile* property) const;
    void renderPlayerProperties(const Player& player) const;
    void renderAuctionUI(const PropertyTile* property, int currentBid, const Player* bidder) const;
};
