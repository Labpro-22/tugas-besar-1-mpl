#include "core/BankruptcyHandler.hpp"
#include "models/Player.hpp"
#include "core/GameContext.hpp"
#include "core/AuctionManager.hpp"
#include "core/GameIO.hpp"
#include "core/TurnManager.hpp"
#include "models/tiles/PropertyTile.hpp"
#include "utils/TransactionLogger.hpp"
#include <string>
#include <vector>

namespace {
    int getCurrentTurn(GameContext& context) {
        TurnManager* turnManager = context.getTurnManager();
        return turnManager == nullptr ? 0 : turnManager->getCurrentTurn();
    }

    std::vector<LiquidationCandidate> buildLiquidationCandidates(Player& player) {
        std::vector<LiquidationCandidate> options;
        for (PropertyTile* property : player.getProperties()) {
            if (property == nullptr || property->getStatus() != PropertyStatus::OWNED) {
                continue;
            }

            LiquidationCandidate candidate;
            candidate.tileIndex = property->getIndex();
            candidate.code = property->getCode();
            candidate.name = property->getName();
            candidate.sellValue = property->getSellValueToBank();
            candidate.mortgageValue = property->getMortgageValue();
            if (candidate.sellValue > 0 || candidate.mortgageValue > 0) {
                options.push_back(candidate);
            }
        }
        return options;
    }

    PropertyTile* findOwnedPropertyByTileIndex(Player& player, int tileIndex) {
        for (PropertyTile* property : player.getProperties()) {
            if (property != nullptr && property->getIndex() == tileIndex) {
                return property;
            }
        }
        return nullptr;
    }

    void resetPropertyDevelopment(PropertyTile* property) {
        if (property != nullptr) {
            property->setBuildingLevel(0);
            property->setFestivalState(1, 0);
        }
    }

    void sellPropertyToBank(Player& player, PropertyTile* property, GameContext& context) {
        if (property == nullptr) {
            return;
        }

        int received = property->getSellValueToBank();
        std::string code = property->getCode();
        resetPropertyDevelopment(property);
        property->returnToBank();
        player += received;
        context.logEvent(
            "LIKUIDASI",
            player.getUsername() + " menjual " + code + " ke Bank dan menerima M" + std::to_string(received));
        if (context.getIO() != nullptr) {
            context.getIO()->showPaymentNotification(
                "LIQUIDATION",
                player.getUsername() + " menjual " + property->getName() +
                " ke Bank dan menerima M" + std::to_string(received) + ".");
            context.getIO()->showMessage(
                player.getUsername() + " menjual " + code + " ke Bank dan menerima M" +
                std::to_string(received) + ".");
        }
    }

    void mortgagePropertyForLiquidation(Player& player, PropertyTile* property, GameContext& context) {
        if (property == nullptr || property->getStatus() != PropertyStatus::OWNED) {
            return;
        }

        int received = property->getMortgageValue();
        property->mortgage();
        player += received;
        context.logEvent(
            "LIKUIDASI",
            player.getUsername() + " menggadaikan " + property->getCode() +
            " dan menerima M" + std::to_string(received));
        if (context.getIO() != nullptr) {
            context.getIO()->showPaymentNotification(
                "LIQUIDATION",
                player.getUsername() + " menggadaikan " + property->getName() +
                " dan menerima M" + std::to_string(received) + ".");
            context.getIO()->showMessage(
                player.getUsername() + " menggadaikan " + property->getCode() +
                " dan menerima M" + std::to_string(received) + ".");
        }
    }
}

bool BankruptcyHandler::handleBankruptcy(Player& player, Player* creditor, int amount, GameContext& context) {
    int totalAvailable = calculateLiquidationMax(player);
    
    if (totalAvailable < amount) {
        std::vector<PropertyTile*> assets = player.getProperties();
        if (creditor) {
            int transferred = player.getBalance();
            creditor->setBalance(creditor->getBalance() + player.getBalance());
            transferAssetsToPlayer(player, *creditor);
            if (context.getIO() != nullptr && transferred > 0) {
                context.getIO()->showPaymentNotification(
                    "PAYMENT",
                    player.getUsername() + " menyerahkan saldo terakhir M" +
                        std::to_string(transferred) + " kepada " + creditor->getUsername() + ".");
            }
        } else {
            transferAssetsToBank(player, context.getAuctionManager());
        }
        player.setBalance(0);
        declareBankrupt(player, context);
        if (creditor == nullptr) {
            auctionBankruptAssets(assets, context);
        }
        return true;
    } else {
        const bool liquidationCompleted = liquidateAssets(player, amount, context);
        if (!liquidationCompleted) {
            if (context.getIO() != nullptr) {
                context.getIO()->showMessage(
                    "Likuidasi aset dibatalkan. Kondisi keuangan dikembalikan seperti sebelum likuidasi.");
            }
            return false;
        }

        if (player.getBalance() < amount) {
            std::vector<PropertyTile*> assets = player.getProperties();
            if (creditor) {
                creditor->setBalance(creditor->getBalance() + player.getBalance());
                transferAssetsToPlayer(player, *creditor);
            } else {
                transferAssetsToBank(player, context.getAuctionManager());
            }
            player.setBalance(0);
            declareBankrupt(player, context);
            if (creditor == nullptr) {
                auctionBankruptAssets(assets, context);
            }
            return true;
        }
        player.setBalance(player.getBalance() - amount);
        if (creditor) {
            creditor->setBalance(creditor->getBalance() + amount);
        }
        if (context.getIO() != nullptr) {
            context.getIO()->showPaymentNotification(
                "PAYMENT",
                player.getUsername() + " membayar M" + std::to_string(amount) +
                    (creditor == nullptr ? " ke Bank." : " kepada " + creditor->getUsername() + "."));
        }
        return true;
    }
}

int BankruptcyHandler::calculateLiquidationMax(Player& player) const {
    return player.getLiquidationMax();
}

bool BankruptcyHandler::liquidateAssets(Player& player, int amount, GameContext& context) {
    if (player.getBalance() >= amount) {
        return true;
    }

    GameIO* io = context.getIO();
    std::vector<LiquidationCandidate> candidates = buildLiquidationCandidates(player);
    if (candidates.empty()) {
        return false;
    }

    if (io == nullptr) {
        while (player.getBalance() < amount) {
            PropertyTile* bestProperty = nullptr;
            LiquidationActionKind bestAction = LiquidationActionKind::Sell;
            int bestValue = -1;

            for (const LiquidationCandidate& candidate : candidates) {
                PropertyTile* property = findOwnedPropertyByTileIndex(player, candidate.tileIndex);
                if (property == nullptr || property->getStatus() != PropertyStatus::OWNED) {
                    continue;
                }

                if (candidate.sellValue > bestValue) {
                    bestProperty = property;
                    bestAction = LiquidationActionKind::Sell;
                    bestValue = candidate.sellValue;
                }
                if (candidate.mortgageValue > bestValue) {
                    bestProperty = property;
                    bestAction = LiquidationActionKind::Mortgage;
                    bestValue = candidate.mortgageValue;
                }
            }

            if (bestProperty == nullptr || bestValue <= 0) {
                return false;
            }

            if (bestAction == LiquidationActionKind::Sell) {
                sellPropertyToBank(player, bestProperty, context);
            } else {
                mortgagePropertyForLiquidation(player, bestProperty, context);
            }
        }

        return true;
    }

    std::vector<LiquidationDecision> decisions;
    if (!io->promptLiquidationPlan(player, amount, candidates, decisions)) {
        return false;
    }

    for (const LiquidationDecision& decision : decisions) {
        PropertyTile* selected = findOwnedPropertyByTileIndex(player, decision.tileIndex);
        if (selected == nullptr || selected->getStatus() != PropertyStatus::OWNED) {
            continue;
        }

        if (decision.action == LiquidationActionKind::Sell) {
            sellPropertyToBank(player, selected, context);
        } else {
            mortgagePropertyForLiquidation(player, selected, context);
        }
    }

    return player.getBalance() >= amount;
}

void BankruptcyHandler::auctionBankruptAssets(
    const std::vector<PropertyTile*>& assets,
    GameContext& context
) {
    AuctionManager* auction = context.getAuctionManager();
    TurnManager* turnManager = context.getTurnManager();
    GameIO* io = context.getIO();
    if (auction == nullptr || turnManager == nullptr) {
        return;
    }

    for (PropertyTile* property : assets) {
        if (property == nullptr || property->getStatus() != PropertyStatus::BANK) {
            continue;
        }

        std::vector<Player*> bidders = turnManager->getActivePlayers();
        if (bidders.size() < 2) {
            return;
        }

        auction->conductAuction(property, bidders, nullptr);
        const std::vector<Player*>& auctionOrder = auction->getAuctionOrder();
        int totalPlayers = static_cast<int>(auctionOrder.size());

        if (io != nullptr) {
            io->showAuctionNotification(
                "AUCTION",
                "Lelang aset bangkrut " + property->getName() +
                " (" + property->getCode() + ") dimulai.");
            io->showMessage(
                "=== Lelang aset bangkrut: " + property->getName() +
                " (" + property->getCode() + ") ===");
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
                    amount = io->promptAuctionBid(*property, *bidder, auction->getHighestBid());
                }

                if (amount < 0) {
                    auction->processPass(bidder);
                    continue;
                }

                try {
                    if (!auction->processBid(bidder, amount) && io != nullptr) {
                        io->showMessage("Bid harus lebih besar dari bid tertinggi.");
                    }
                } catch (const std::exception& e) {
                    if (io != nullptr) {
                        io->showError(e, context.getLogger(), getCurrentTurn(context), bidder->getUsername());
                    }
                }
            }

            if (auction->isFinished(totalPlayers) && auction->getHighestBidder() == nullptr) {
                Player* forcedBidder = auction->getLastNonPasser();
                if (forcedBidder == nullptr || forcedBidder->isBankrupt()) {
                    break;
                }

                int amount = 0;
                if (io != nullptr) {
                    io->showMessage(
                        "Belum ada pemain yang melakukan bid. " +
                        forcedBidder->getUsername() + " wajib melakukan bid awal.");
                    amount = io->promptAuctionBid(*property, *forcedBidder, auction->getHighestBid());
                    if (amount < 0) {
                        amount = 0;
                    }
                }

                try {
                    auction->processBid(forcedBidder, amount);
                } catch (const std::exception& e) {
                    if (io != nullptr) {
                        io->showError(e, context.getLogger(), getCurrentTurn(context), forcedBidder->getUsername());
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
                winner->getUsername() + " memenangkan aset bangkrut " + property->getCode() +
                " seharga M" + std::to_string(winningBid));
            if (io != nullptr) {
                io->showPaymentNotification(
                    "PAYMENT",
                    winner->getUsername() + " membayar M" + std::to_string(winningBid) +
                        " ke Bank untuk memenangkan " + property->getName() + ".");
                io->showAuctionNotification(
                    "AUCTION",
                    winner->getUsername() + " memenangkan " + property->getName() +
                        " seharga M" + std::to_string(winningBid) + ".");
                io->showMessage(
                    winner->getUsername() + " memenangkan " + property->getName() +
                    " seharga M" + std::to_string(winningBid) + ".");
            }
        } else {
            context.logEvent("LELANG", property->getCode() + " tidak terjual pada lelang aset bangkrut.");
            if (io != nullptr) {
                io->showAuctionNotification(
                    "AUCTION",
                    "Lelang selesai. " + property->getName() + " tidak terjual.");
                io->showMessage(property->getName() + " tidak terjual.");
            }
        }
    }
}

void BankruptcyHandler::transferAssetsToPlayer(Player& from, Player& to) {
    std::vector<PropertyTile*> properties = from.getProperties();
    for (PropertyTile* prop : properties) {
        if (prop != nullptr) {
            bool wasMortgaged = prop->getStatus() == PropertyStatus::MORTGAGED;
            prop->transferTo(to);
            if (wasMortgaged) {
                prop->mortgage();
            }
        }
    }
}

void BankruptcyHandler::transferAssetsToBank(Player& player, AuctionManager* auction) {
    if (auction != nullptr) {
        auction->reset();
    }
    std::vector<PropertyTile*> properties = player.getProperties();
    for (PropertyTile* prop : properties) {
        if (prop != nullptr) {
            prop->setBuildingLevel(0);
            prop->setFestivalState(1, 0);
            prop->returnToBank();
        }
    }
}

void BankruptcyHandler::declareBankrupt(Player& player, GameContext& context) {
    player.setStatus(PlayerStatus::BANKRUPT);
    if (context.getIO() != nullptr) {
        context.getIO()->showMessage(player.getUsername() + " dinyatakan BANGKRUT!");
    }
    context.logEvent("BANGKRUT", player.getUsername() + " dinyatakan bangkrut.");
    if (context.getTurnManager() != nullptr) {
        context.getTurnManager()->removePlayer(&player);
    }
}
