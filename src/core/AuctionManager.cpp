#include "core/AuctionManager.hpp"

#include <algorithm>
#include <stdexcept>

#include "models/Player.hpp"
#include "models/tiles/PropertyTile.hpp"
#include "utils/exceptions/NimonspoliException.hpp"


AuctionManager::AuctionManager()
    : property(nullptr),
      highestBid(-1),
      highestBidder(nullptr),
      consecutivePasses(0),
      lastNonPasser(nullptr) {}

void AuctionManager::conductAuction(PropertyTile* prop,
                                    const std::vector<Player*>& players,
                                    Player* startingPlayer) {
    reset();
    this->property = prop;

    if (players.empty() || prop == nullptr) return;

    int startIdx = 0;
    if (startingPlayer != nullptr) {
        auto it = std::find(players.begin(), players.end(), startingPlayer);
        if (it != players.end()) {
            startIdx = static_cast<int>(std::distance(players.begin(), it) + 1)
                       % static_cast<int>(players.size());
        }
    }

    auctionOrder.clear();
    int n = static_cast<int>(players.size());
    for (int i = 0; i < n; ++i) {
        auctionOrder.push_back(players[(startIdx + i) % n]);
    }
}

bool AuctionManager::processBid(Player* player, int amount) {
    if (property == nullptr || player == nullptr) {
        return false;
    }

    if (amount < 0) {
        return false;
    }

    if (highestBidder != nullptr && amount <= highestBid) {
        return false;
    }

    if (!player->canAfford(amount)) {
        throw InsufficientFundsException(amount, player->getBalance());
    }

    highestBid       = amount;
    highestBidder    = player;
    consecutivePasses = 0;
    return true;
}


void AuctionManager::processPass(Player* player) {
    if (highestBidder == nullptr && !auctionOrder.empty()) {
        auto it = std::find(auctionOrder.begin(), auctionOrder.end(), player);
        if (it != auctionOrder.end()) {
            int idx = static_cast<int>(std::distance(auctionOrder.begin(), it));
            int nextIdx = (idx + 1) % static_cast<int>(auctionOrder.size());
            lastNonPasser = auctionOrder[nextIdx];
        }
    }

    ++consecutivePasses;
}


bool AuctionManager::isFinished(int totalActivePlayers) const {
    if (totalActivePlayers <= 1) {
        return true;
    }

    if (highestBidder == nullptr) {
        return consecutivePasses >= (totalActivePlayers - 1);
    }

    return consecutivePasses >= (totalActivePlayers - 1);
}

void AuctionManager::finalizeAuction() {
    if (property == nullptr || highestBidder == nullptr) {
        return;
    }

    (*highestBidder) -= highestBid;
    property->transferTo(*highestBidder);
}

void AuctionManager::reset() {
    property          = nullptr;
    highestBid        = -1;
    highestBidder     = nullptr;
    consecutivePasses = 0;
    lastNonPasser     = nullptr;
    auctionOrder.clear();
}

PropertyTile* AuctionManager::getProperty() const {
    return property;
}

int AuctionManager::getHighestBid() const {
    return highestBid;
}

Player* AuctionManager::getHighestBidder() const {
    return highestBidder;
}

const std::vector<Player*>& AuctionManager::getAuctionOrder() const {
    return auctionOrder;
}

Player* AuctionManager::getLastNonPasser() const {
    return lastNonPasser;
}
