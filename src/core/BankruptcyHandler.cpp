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
    enum class LiquidationAction {
        SELL,
        MORTGAGE
    };

    struct LiquidationOption {
        LiquidationAction action;
        PropertyTile* property;
        int value;
    };

    int getCurrentTurn(GameContext& context) {
        TurnManager* turnManager = context.getTurnManager();
        return turnManager == nullptr ? 0 : turnManager->getCurrentTurn();
    }

    std::vector<LiquidationOption> buildLiquidationOptions(Player& player) {
        std::vector<LiquidationOption> options;
        for (PropertyTile* property : player.getProperties()) {
            if (property == nullptr || property->getStatus() != PropertyStatus::OWNED) {
                continue;
            }

            int sellValue = property->getSellValueToBank();
            if (sellValue > 0) {
                options.push_back({LiquidationAction::SELL, property, sellValue});
            }

            int mortgageValue = property->getMortgageValue();
            if (mortgageValue > 0) {
                options.push_back({LiquidationAction::MORTGAGE, property, mortgageValue});
            }
        }
        return options;
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
            context.getIO()->showMessage(
                player.getUsername() + " menggadaikan " + property->getCode() +
                " dan menerima M" + std::to_string(received) + ".");
        }
    }
}

void BankruptcyHandler::handleBankruptcy(Player& player, Player* creditor, int amount, GameContext& context) {
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
    } else {
        liquidateAssets(player, amount, context);
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
            return;
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
    while (player.getBalance() < amount) {
        std::vector<LiquidationOption> options = buildLiquidationOptions(player);
        if (options.empty()) {
            return false;
        }

        if (io == nullptr) {
            LiquidationOption best = options.front();
            for (const LiquidationOption& option : options) {
                if (option.value > best.value) {
                    best = option;
                }
            }

            if (best.action == LiquidationAction::SELL) {
                sellPropertyToBank(player, best.property, context);
            } else {
                mortgagePropertyForLiquidation(player, best.property, context);
            }
            continue;
        }

        io->showMessage(
            "Saldo M" + std::to_string(player.getBalance()) +
            " belum cukup untuk membayar M" + std::to_string(amount) + ".");
        io->showMessage("Pilih aset untuk dilikuidasi:");
        for (int i = 0; i < static_cast<int>(options.size()); ++i) {
            std::string actionLabel = options[i].action == LiquidationAction::SELL
                ? "Jual ke Bank"
                : "Gadai";
            io->showMessage(
                std::to_string(i + 1) + ". " + actionLabel + " " +
                options[i].property->getName() + " (" + options[i].property->getCode() +
                ") - M" + std::to_string(options[i].value));
        }

        int choice = io->promptIntInRange(
            "Pilih opsi likuidasi (1-" + std::to_string(options.size()) + "): ",
            1,
            static_cast<int>(options.size()));

        LiquidationOption selected = options[choice - 1];
        if (selected.action == LiquidationAction::SELL) {
            sellPropertyToBank(player, selected.property, context);
        } else {
            mortgagePropertyForLiquidation(player, selected.property, context);
        }
    }

    return true;
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
                    int displayedHighestBid = auction->getHighestBidder() == nullptr
                        ? 0
                        : auction->getHighestBid();
                    amount = io->promptInt(
                        bidder->getUsername() +
                        " saldo M" + std::to_string(bidder->getBalance()) +
                        ", bid tertinggi M" + std::to_string(displayedHighestBid) +
                        ". Masukkan bid (-1 untuk pass): ");
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
                    amount = io->promptIntInRange(
                        forcedBidder->getUsername() +
                        " saldo M" + std::to_string(forcedBidder->getBalance()) +
                        ". Masukkan bid awal (0-" +
                        std::to_string(forcedBidder->getBalance()) + "): ",
                        0,
                        forcedBidder->getBalance());
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
                io->showMessage(
                    winner->getUsername() + " memenangkan " + property->getName() +
                    " seharga M" + std::to_string(winningBid) + ".");
            }
        } else {
            context.logEvent("LELANG", property->getCode() + " tidak terjual pada lelang aset bangkrut.");
            if (io != nullptr) {
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
