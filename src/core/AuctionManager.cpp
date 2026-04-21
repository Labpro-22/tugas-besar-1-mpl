#include "core/AuctionManager.hpp"

#include <stdexcept>

#include "models/Player.hpp"
#include "models/tiles/PropertyTile.hpp"
#include "utils/exceptions/InsufficientFundsException.hpp"

AuctionManager::AuctionManager()
    : property(nullptr), highestBid(0), highestBidder(nullptr), consecutivePasses(0) {}

void AuctionManager::conductAuction(PropertyTile* property, const std::vector<Player*>& players, Player* startingPlayer) {
  (void) players;
  (void) startingPlayer;

  reset();
  this->property = property;
}

bool AuctionManager::processBid(Player* player, int amount) {
  if (property == nullptr || player == nullptr) {
    return false;
  }

  if (amount <= highestBid) {
    return false;
  }

  if (!player->canAfford(amount)) {
    throw InsufficientFundsException(amount, player->getBalance());
  }

  highestBid = amount;
  highestBidder = player;
  consecutivePasses = 0;
  return true;
}

void AuctionManager::processPass(Player* player) {
  (void) player;
  ++consecutivePasses;
}

bool AuctionManager::isFinished(int totalActivePlayers) const {
  if (totalActivePlayers <= 1) {
    return true;
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
  property = nullptr;
  highestBid = 0;
  highestBidder = nullptr;
  consecutivePasses = 0;
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