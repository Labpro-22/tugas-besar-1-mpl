#include "core/TurnManager.hpp"

#include <algorithm>
#include <stdexcept>

#include "core/AuctionManager.hpp"
#include "core/BankruptcyHandler.hpp"
#include "core/Dice.hpp"
#include "core/GameContext.hpp"
#include "core/GameIO.hpp"
#include "models/Player.hpp"
#include "models/tiles/PropertyTile.hpp"
#include "utils/TransactionLogger.hpp"

TurnManager::TurnManager(int maxTurn)
    : currentIndex(0), currentTurn(1), maxTurn(maxTurn) {}

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

void TurnManager::restoreTurnOrder(const std::vector<std::string>& orderedUsernames,
                                   const std::vector<Player*>& allPlayers) {
    turnOrder.clear();

    for (const std::string& username : orderedUsernames) {
        for (Player* player : allPlayers) {
            if (player != nullptr &&
                player->getUsername() == username &&
                !player->isBankrupt()) {
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

    auto it = std::find(turnOrder.begin(), turnOrder.end(), player);
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
        throw std::out_of_range("TurnManager::setCurrentIndex index di luar batas.");
    }

    currentIndex = index;
}

void TurnManager::handlePropertyLanded(Player& player,
                                       PropertyTile& tile,
                                       GameContext& context) {
    if (tile.getStatus() == PropertyStatus::BANK) {
        int price = tile.getBuyPrice();

        if (price <= 0 || player.canAfford(price)) {
            if (price > 0) {
                player -= price;
            }

            tile.transferTo(player);
            context.logEvent(
                "BELI",
                player.getUsername() + " membeli " + tile.getName() +
                    " seharga M" + std::to_string(price));
            return;
        }

        AuctionManager* auction = context.getAuctionManager();
        if (auction == nullptr) {
            context.logEvent(
                "LELANG",
                tile.getName() + " belum terbeli karena saldo tidak cukup.");
            return;
        }

        std::vector<Player*> activePlayers = getActivePlayers();
        auction->conductAuction(&tile, activePlayers, &player);

        std::vector<Player*> auctionOrder = auction->getAuctionOrder();
        int totalPlayers = static_cast<int>(auctionOrder.size());
        GameIO* io = context.getIO();

        if (io != nullptr) {
            io->showMessage("=== Lelang " + tile.getName() + " (" + tile.getCode() + ") ===");
        }

        while (!auction->isFinished(totalPlayers)) {
            for (Player* bidder : auctionOrder) {
                if (bidder == nullptr || bidder->isBankrupt()) {
                    continue;
                }

                if (auction->isFinished(totalPlayers)) {
                    break;
                }

                int amount = 0;
                if (io != nullptr) {
                    amount = io->promptInt(
                        bidder->getUsername() +
                            " saldo M" + std::to_string(bidder->getBalance()) +
                            ", bid tertinggi M" + std::to_string(auction->getHighestBid()) +
                            ". Masukkan bid (0 untuk pass): ");
                }

                if (amount <= 0) {
                    auction->processPass(bidder);
                    continue;
                }

                try {
                    if (!auction->processBid(bidder, amount)) {
                        if (io != nullptr) {
                            io->showMessage("Bid harus lebih besar dari bid tertinggi.");
                        }
                    }
                } catch (const std::exception& e) {
                    if (io != nullptr) {
                        io->showError(e, context.getLogger(), getCurrentTurn(), bidder->getUsername());
                    }
                }
            }
        }

        Player* winner = auction->getHighestBidder();
        int winningBid = auction->getHighestBid();
        auction->finalizeAuction();

        if (winner != nullptr) {
            context.logEvent(
                "LELANG",
                winner->getUsername() + " memenangkan " + tile.getName() +
                    " seharga M" + std::to_string(winningBid));
        } else {
            context.logEvent("LELANG", tile.getName() + " tidak terjual.");
        }

        return;
    }

    if (tile.getStatus() == PropertyStatus::OWNED && !tile.isOwnedBy(player)) {
        handleRentPayment(player, tile, context);
    }
}

void TurnManager::handleRentPayment(Player& player,
                                    PropertyTile& tile,
                                    GameContext& context) {
    if (tile.isMortgaged() || tile.isOwnedBy(player)) {
        return;
    }

    if (player.isShieldActive()) {
        context.logEvent(
            "SEWA",
            player.getUsername() + " terlindungi ShieldCard dari tagihan sewa.");
        return;
    }

    int lastDiceTotal = 0;
    Dice* dice = context.getDice();
    if (dice != nullptr) {
        lastDiceTotal = dice->getTotal();
    }

    int rentAmount = tile.calculateRent(lastDiceTotal, context);

    if (player.getDiscountPercent() > 0) {
        rentAmount -= (rentAmount * player.getDiscountPercent()) / 100;
    }

    if (rentAmount <= 0) {
        return;
    }

    Player* owner = tile.getOwner();
    if (owner == nullptr) {
        return;
    }

    if (!player.canAfford(rentAmount)) {
        BankruptcyHandler* bankruptcyHandler = context.getBankruptcyHandler();
        if (bankruptcyHandler != nullptr) {
            bankruptcyHandler->handleBankruptcy(player, owner, rentAmount, context);
        }
        return;
    }

    player -= rentAmount;
    *owner += rentAmount;

    context.logEvent(
        "SEWA",
        player.getUsername() + " membayar sewa ke " + owner->getUsername() +
            " sebesar M" + std::to_string(rentAmount));
}
