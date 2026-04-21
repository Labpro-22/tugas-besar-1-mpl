#include "core/TurnManager.hpp"

#include <algorithm>
#include <stdexcept>

#include "models/Player.hpp"
#include "core/GameContext.hpp"
#include "models/tiles/PropertyTile.hpp"

TurnManager::TurnManager(int maxTurn)
    : currentIndex(0),
      currentTurn(1),
      maxTurn(maxTurn) {}


void TurnManager::initializeTurnOrder(const std::vector<Player*>& players) {
  turnOrder.clear();

  for (Player* player : players) {
    if (player != nullptr && !player->isBankrupt()) {
      turnOrder.push_back(player);
    }
  }

  currentIndex = 0;
  currentTurn = 1;
}

void TurnManager::restoreTurnOrder(const std::vector<std::string>& orderedUsernames, const std::vector<Player*>& allPlayers) {
  turnOrder.clear();
  
  for (const std::string& username : orderedUsernames) {
    for (Player* player : allPlayers) {
      if (player != nullptr && player->getUsername() == username && !player->isBankrupt()) {
        turnOrder.push_back(player);
        break;
      }
    }
  }

  currentIndex = 0;
  if (turnOrder.empty()) {
    currentTurn = 1;
  }
}

Player* TurnManager::getCurrentPlayer() const {
  if (turnOrder.empty()) {
    return nullptr;
  }

  return turnOrder[currentIndex];
}

void TurnManager::advance() {
  if (turnOrder.empty()) {
    return;
  }

  currentIndex = (currentIndex + 1) % static_cast<int>(turnOrder.size());

  if (currentIndex == 0) {
    ++currentTurn;
  }
}

bool TurnManager::isMaxTurnReached() const {
  return currentTurn >= maxTurn;
}

void TurnManager::removePlayer(Player* player) {
  if (player == nullptr || turnOrder.empty()) {
    return;
  }

  std::vector<Player*>::iterator it = std::find(turnOrder.begin(), turnOrder.end(), player);

  if (it == turnOrder.end()) {
    return;
  }

  int removedIndex = static_cast<int>(it - turnOrder.begin());
  turnOrder.erase(it);

  if (turnOrder.empty()) {
    currentIndex = 0;
    return;
  }

  if (removedIndex < currentIndex) {
    --currentIndex;
  } else if (removedIndex == currentIndex) {
    if (currentIndex >= static_cast<int>(turnOrder.size())) {
      currentIndex = 0;
    }
  }
}

std::vector<Player*> TurnManager::getActivePlayers() const {
  return turnOrder;
}

Player* TurnManager::getPlayerAfter(Player* player) const {
  if (player == nullptr || turnOrder.empty()) {
    return nullptr;
  }

  for (int i = 0; i < static_cast<int>(turnOrder.size()); ++i) {
    if (turnOrder[i] == player) {
      return turnOrder[(i + 1) % static_cast<int>(turnOrder.size())];
    }
  }

  return nullptr;
}

int TurnManager::getCurrentTurn() const {
  return currentTurn;
}

int TurnManager::getMaxTurn() const {
  return maxTurn;
}

void TurnManager::setCurrentTurn(int turn) {
  currentTurn = (turn < 1) ? 1 : turn;
}

void TurnManager::setCurrentIndex(int index) {
  if (turnOrder.empty()) {
    currentIndex = 0;
    return;
  }

  if (index < 0 || index >= static_cast<int>(turnOrder.size())) {
    throw std::out_of_range("TurnManager::setCurrentIndex index di luat batas.");
  }

  currentIndex = index;
}

void TurnManager::handlePropertyLanded(Player& player, PropertyTile& tile, GameContext& context) {
    if (tile.getStatus() == PropertyStatus::BANK) {
        // [TODO: Panggil UI/Controller untuk opsi beli]
        // Jika pemain tidak beli / tidak cukup uang -> panggil context.getAuctionManager()->startAuction(tile);
    } else if (tile.getStatus() == PropertyStatus::OWNED && !tile.isOwnedBy(player)) {
        handleRentPayment(player, tile, context);
    }
}

void TurnManager::handleRentPayment(Player& player, PropertyTile& tile, GameContext& context) {
    if (tile.isMortgaged() || tile.isOwnedBy(player)) {
        return;
    }

    int lastDiceTotal = 0; 
    // if (context.getDice()) {
    //     lastDiceTotal = context.getDice()->getTotal(); 
    // }

    int rentAmount = tile.calculateRent(lastDiceTotal, context);
    
    if (rentAmount > 0) {
        Player* owner = tile.getOwner();
        // [TODO: Lakukan proses transfer atau masuk ke BankruptcyHandler jika uang player tidak cukup]
        context.logEvent("SEWA", player.getUsername() + " membayar sewa ke " + owner->getUsername() + " sebesar M" + std::to_string(rentAmount));
    }
}