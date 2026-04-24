#include "core/BankruptcyHandler.hpp"
#include "models/Player.hpp"
#include "core/GameContext.hpp"
#include "core/AuctionManager.hpp"
#include "core/GameIO.hpp"
#include "core/TurnManager.hpp"
#include "models/tiles/PropertyTile.hpp"
#include "utils/OutputFormatter.hpp"
#include "utils/TransactionLogger.hpp"
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

namespace {
    std::string fitColumn(const std::string& text, int width) {
        if (static_cast<int>(text.size()) >= width) {
            return text.substr(0, width);
        }
        return text + std::string(width - text.size(), ' ');
    }

    enum class LiquidationAction {
        SELL,
        MORTGAGE
    };

    struct LiquidationOption {
        LiquidationAction action;
        PropertyTile* property;
        int value;
    };

    std::string propertyStateLabel(const PropertyTile& property) {
        std::string status = property.getStatus() == PropertyStatus::MORTGAGED ? "MORTGAGED [M]" : "OWNED";
        if (property.getPropertyType() != PropertyType::STREET) {
            return status;
        }

        int level = property.getBuildingLevel();
        if (level == 5) {
            return status + " (Hotel)";
        }
        if (level > 0) {
            return status + " (" + std::to_string(level) + " rumah)";
        }
        return status;
    }

    std::string liquidationActionLabel(LiquidationAction action) {
        return action == LiquidationAction::SELL ? "Jual" : "Gadai";
    }

    std::vector<LiquidationOption> buildEstimatedLiquidationOptions(Player& player) {
        std::vector<LiquidationOption> estimates;
        for (PropertyTile* property : player.getProperties()) {
            if (property == nullptr || property->getStatus() != PropertyStatus::OWNED) {
                continue;
            }

            int sellValue = property->getSellValueToBank();
            int mortgageValue = property->getMortgageValue();
            if (sellValue <= 0 && mortgageValue <= 0) {
                continue;
            }

            if (sellValue >= mortgageValue) {
                estimates.push_back({LiquidationAction::SELL, property, sellValue});
            } else {
                estimates.push_back({LiquidationAction::MORTGAGE, property, mortgageValue});
            }
        }
        return estimates;
    }

    void showLiquidationEstimate(Player& player, int amount, GameContext& context) {
        GameIO* io = context.getIO();
        if (io == nullptr) {
            return;
        }

        int balance = player.getBalance();
        io->showMessage("Uang kamu       : " + OutputFormatter::formatMoney(balance));
        io->showMessage("Total kewajiban : " + OutputFormatter::formatMoney(amount));
        if (amount > balance) {
            io->showMessage("Kekurangan      : " + OutputFormatter::formatMoney(amount - balance));
        }
        io->showMessage("");
        io->showMessage("Estimasi dana maksimum dari likuidasi:");

        std::vector<LiquidationOption> estimates = buildEstimatedLiquidationOptions(player);
        if (estimates.empty()) {
            io->showMessage("  Tidak ada aset yang dapat dilikuidasi.");
            return;
        }

        if (static_cast<int>(estimates.size()) <= 4) {
            int totalPotential = 0;
            for (const LiquidationOption& option : estimates) {
                std::ostringstream line;
                line << "  " << fitColumn(liquidationActionLabel(option.action), 5)
                     << fitColumn(option.property->getName() + " (" + option.property->getCode() + ")", 18)
                     << "[" << OutputFormatter::formatPropertyCategory(*option.property) << "]"
                     << "   -> " << OutputFormatter::formatMoney(option.value);
                io->showMessage(line.str());
                totalPotential += option.value;
            }
            io->showMessage("  Total potensi        -> " + OutputFormatter::formatMoney(totalPotential));
            return;
        }

        int totalPotential = player.getLiquidationMax() - balance;
        io->showMessage("  Jual semua properti + bangunan -> " + OutputFormatter::formatMoney(totalPotential));
        io->showMessage("Total aset + uang tunai          : " + OutputFormatter::formatMoney(player.getLiquidationMax()));
    }

    void showLiquidationPanel(Player& player, int amount, const std::vector<LiquidationOption>& options, GameContext& context) {
        GameIO* io = context.getIO();
        if (io == nullptr) {
            return;
        }

        io->showMessage("=== Panel Likuidasi ===");
        io->showMessage(
            "Uang kamu saat ini: " + OutputFormatter::formatMoney(player.getBalance()) +
            "  |  Kewajiban: " + OutputFormatter::formatMoney(amount)
        );
        io->showMessage("");
        io->showMessage("[Jual ke Bank]");
        for (int i = 0; i < static_cast<int>(options.size()); ++i) {
            if (options[i].action != LiquidationAction::SELL) {
                continue;
            }
            std::ostringstream line;
            line << (i + 1) << ". "
                 << fitColumn(options[i].property->getName() + " (" + options[i].property->getCode() + ")", 18)
                 << "[" << OutputFormatter::formatPropertyCategory(*options[i].property) << "]  Harga Jual: "
                 << OutputFormatter::formatMoney(options[i].value);
            io->showMessage(line.str());
        }
        io->showMessage("");
        io->showMessage("[Gadaikan]");
        for (int i = 0; i < static_cast<int>(options.size()); ++i) {
            if (options[i].action != LiquidationAction::MORTGAGE) {
                continue;
            }
            std::ostringstream line;
            line << (i + 1) << ". "
                 << fitColumn(options[i].property->getName() + " (" + options[i].property->getCode() + ")", 18)
                 << "[" << OutputFormatter::formatPropertyCategory(*options[i].property) << "]  Nilai Gadai: "
                 << OutputFormatter::formatMoney(options[i].value);
            io->showMessage(line.str());
        }
    }

    std::vector<std::string> describePlayerAssets(const Player& player) {
        std::vector<std::string> lines;
        for (PropertyTile* prop : player.getProperties()) {
            if (prop == nullptr) {
                continue;
            }
            std::ostringstream line;
            line << "  - " << fitColumn(prop->getName() + " (" + prop->getCode() + ")", 18)
                 << "[" << OutputFormatter::formatPropertyCategory(*prop) << "]  "
                 << propertyStateLabel(*prop);
            lines.push_back(line.str());
        }
        return lines;
    }

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
                property->getName() + " terjual ke Bank. Kamu menerima " + OutputFormatter::formatMoney(received) + ".");
            context.getIO()->showMessage("Uang kamu sekarang: " + OutputFormatter::formatMoney(player.getBalance()));
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
                property->getName() + " berhasil digadaikan. Kamu menerima " + OutputFormatter::formatMoney(received) + ".");
            context.getIO()->showMessage("Uang kamu sekarang: " + OutputFormatter::formatMoney(player.getBalance()));
        }
    }
}

void BankruptcyHandler::handleBankruptcy(Player& player, Player* creditor, int amount, GameContext& context) {
    GameIO* io = context.getIO();
    int totalAvailable = calculateLiquidationMax(player);

    if (io != nullptr) {
        io->showMessage("");
        showLiquidationEstimate(player, amount, context);
        io->showMessage("");
    }

    if (totalAvailable < amount) {
        std::vector<PropertyTile*> assets = player.getProperties();
        std::vector<std::string> assetDescriptions = describePlayerAssets(player);
        if (io != nullptr) {
            io->showMessage("Tidak cukup untuk menutup kewajiban " + OutputFormatter::formatMoney(amount) + ".");
            io->showMessage("");
            io->showMessage(player.getUsername() + " dinyatakan BANGKRUT!");
            io->showMessage("Kreditor: " + std::string(creditor == nullptr ? "Bank" : creditor->getUsername()));
            io->showMessage("");
        }
        if (creditor) {
            if (io != nullptr) {
                io->showMessage("Pengalihan aset ke " + creditor->getUsername() + ":");
                io->showMessage("  - Uang tunai sisa  : " + OutputFormatter::formatMoney(player.getBalance()));
                for (const std::string& line : assetDescriptions) {
                    io->showMessage(line);
                }
                io->showMessage("");
            }
            creditor->setBalance(creditor->getBalance() + player.getBalance());
            transferAssetsToPlayer(player, *creditor);
            if (io != nullptr) {
                io->showMessage(creditor->getUsername() + " menerima semua aset " + player.getUsername() + ".");
            }
        } else {
            if (io != nullptr) {
                io->showMessage("Uang sisa " + OutputFormatter::formatMoney(player.getBalance()) + " diserahkan ke Bank.");
                io->showMessage("Seluruh properti dikembalikan ke status BANK.");
                io->showMessage("Bangunan dihancurkan - stok dikembalikan ke Bank.");
                io->showMessage("");
                io->showMessage("Properti akan dilelang satu per satu:");
                for (PropertyTile* property : assets) {
                    if (property != nullptr) {
                        io->showMessage("  -> Lelang: " + property->getName() + " (" + property->getCode() + ") ...");
                    }
                }
                io->showMessage("");
            }
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
        if (io != nullptr && creditor != nullptr) {
            io->showMessage("");
            io->showMessage("Kewajiban " + OutputFormatter::formatMoney(amount) + " terpenuhi. Membayar ke " + creditor->getUsername() + "...");
            io->showMessage(
                "Uang kamu : " + OutputFormatter::formatMoney(player.getBalance()) + " -> " + OutputFormatter::formatMoney(player.getBalance() - amount)
            );
            io->showMessage(
                "Uang " + creditor->getUsername() + ": " + OutputFormatter::formatMoney(creditor->getBalance()) +
                " -> " + OutputFormatter::formatMoney(creditor->getBalance() + amount)
            );
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
                "Properti " + property->getName() + " (" + property->getCode() + ") akan dilelang!");
            io->showMessage("");
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
                    } else if (io != nullptr) {
                        io->showMessage(
                            "Penawaran tertinggi: " + OutputFormatter::formatMoney(auction->getHighestBid()) +
                            " (" + bidder->getUsername() + ")"
                        );
                        io->showMessage("");
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
                    if (io != nullptr) {
                        io->showMessage(
                            "Penawaran tertinggi: " + OutputFormatter::formatMoney(auction->getHighestBid()) +
                            " (" + forcedBidder->getUsername() + ")"
                        );
                        io->showMessage("");
                    }
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
                io->showMessage("Lelang selesai!");
                io->showMessage("Pemenang: " + winner->getUsername());
                io->showMessage("Harga akhir: " + OutputFormatter::formatMoney(winningBid));
                io->showMessage("");
                io->showPaymentNotification(
                    "PAYMENT",
                    winner->getUsername() + " membayar M" + std::to_string(winningBid) +
                        " ke Bank untuk memenangkan " + property->getName() + ".");
                io->showAuctionNotification(
                    "AUCTION",
                    winner->getUsername() + " memenangkan " + property->getName() +
                        " seharga M" + std::to_string(winningBid) + ".");
                io->showMessage(
                    "Properti " + property->getName() + " (" + property->getCode() +
                    ") kini dimiliki " + winner->getUsername() + "."
                );
            }
        } else {
            context.logEvent("LELANG", property->getCode() + " tidak terjual pada lelang aset bangkrut.");
            if (io != nullptr) {
                io->showMessage("Lelang selesai!");
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
    context.logEvent("BANGKRUT", player.getUsername() + " dinyatakan bangkrut.");
    if (context.getTurnManager() != nullptr) {
        context.getTurnManager()->removePlayer(&player);
    }
}
