#pragma once

#include <vector>

class PropertyTile;
class Player;

class AuctionManager {
private:
    PropertyTile* property;
    int highestBid;
    Player* highestBidder;
    int consecutivePasses;

public:
    AuctionManager();

    void conductAuction(PropertyTile* property, const std::vector<Player*>& players, Player* startingPlayer);
    bool processBid(Player* player, int amount);
    void processPass(Player* player);
    bool isFinished(int totalActivePlayers) const;
    void finalizeAuction();

    void reset();

    PropertyTile* getProperty() const;
    int getHighestBid() const;
    Player* getHighestBidder() const;
};
