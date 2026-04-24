#include "core/BankruptcyHandler.hpp"

#include "core/AuctionManager.hpp"
#include "core/GameContext.hpp"
#include "core/GameIO.hpp"
#include "core/TurnManager.hpp"
#include "models/Player.hpp"
#include "models/tiles/PropertyTile.hpp"
#include "models/tiles/StreetTile.hpp"
#include "utils/TextFormatter.hpp"

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

    bool colorGroupHasBuildings(const Player& player, const StreetTile& selectedStreet) {
        for (PropertyTile* property : player.getProperties()) {
            const StreetTile* street = property == nullptr ? nullptr : property->asStreetTile();
            if (street == nullptr || street->getColorGroup() != selectedStreet.getColorGroup()) {
                continue;
            }
            if (street->getBuildingLevel() > 0) {
                return true;
            }
        }
        return false;
    }

    int mortgageValueForLiquidation(const Player& player, PropertyTile* property) {
        if (property == nullptr) {
            return 0;
        }

        const StreetTile* street = property->asStreetTile();
        if (street != nullptr && colorGroupHasBuildings(player, *street)) {
            return 0;
        }

        return property->getMortgageValue();
    }

    std::vector<LiquidationOption> buildEstimatedLiquidationOptions(Player& player) {
        std::vector<LiquidationOption> estimates;
        for (PropertyTile* property : player.getProperties()) {
            if (property == nullptr || property->getStatus() != PropertyStatus::OWNED) {
                continue;
            }

            int sellValue = property->getSellValueToBank();
            int mortgageValue = mortgageValueForLiquidation(player, property);
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
        io->showMessage("Uang kamu       : " + TextFormatter::formatMoney(balance));
        io->showMessage("Total kewajiban : " + TextFormatter::formatMoney(amount));
        if (amount > balance) {
            io->showMessage("Kekurangan      : " + TextFormatter::formatMoney(amount - balance));
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
                     << "[" << TextFormatter::formatPropertyCategory(*option.property) << "]"
                     << "   -> " << TextFormatter::formatMoney(option.value);
                io->showMessage(line.str());
                totalPotential += option.value;
            }
            io->showMessage("  Total potensi        -> " + TextFormatter::formatMoney(totalPotential));
            return;
        }

        int totalPotential = player.getLiquidationMax() - balance;
        io->showMessage("  Jual semua properti + bangunan -> " + TextFormatter::formatMoney(totalPotential));
        io->showMessage("Total aset + uang tunai          : " + TextFormatter::formatMoney(player.getLiquidationMax()));
    }

    std::vector<std::string> describePlayerAssets(const Player& player) {
        std::vector<std::string> lines;
        for (PropertyTile* prop : player.getProperties()) {
            if (prop == nullptr) {
                continue;
            }
            std::ostringstream line;
            line << "  - " << fitColumn(prop->getName() + " (" + prop->getCode() + ")", 18)
                 << "[" << TextFormatter::formatPropertyCategory(*prop) << "]  "
                 << propertyStateLabel(*prop);
            lines.push_back(line.str());
        }
        return lines;
    }

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

            int mortgageValue = mortgageValueForLiquidation(player, property);
            if (mortgageValue > 0) {
                options.push_back({LiquidationAction::MORTGAGE, property, mortgageValue});
            }
        }
        return options;
    }

    std::vector<LiquidationCandidate> buildLiquidationCandidates(Player& player) {
        std::vector<LiquidationCandidate> candidates;
        for (PropertyTile* property : player.getProperties()) {
            if (property == nullptr || property->getStatus() != PropertyStatus::OWNED) {
                continue;
            }

            LiquidationCandidate candidate;
            candidate.tileIndex = property->getIndex();
            candidate.code = property->getCode();
            candidate.name = property->getName();
            candidate.sellValue = property->getSellValueToBank();
            candidate.mortgageValue = mortgageValueForLiquidation(player, property);
            if (candidate.sellValue > 0 || candidate.mortgageValue > 0) {
                candidates.push_back(candidate);
            }
        }
        return candidates;
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
            player.getUsername() + " menjual " + code + " ke Bank dan menerima " +
            TextFormatter::formatMoney(received));
        if (context.getIO() != nullptr) {
            context.getIO()->showPaymentNotification(
                "LIQUIDATION",
                player.getUsername() + " menjual " + property->getName() +
                " ke Bank dan menerima " + TextFormatter::formatMoney(received) + ".");
            context.getIO()->showMessage(
                property->getName() + " terjual ke Bank. Kamu menerima " + TextFormatter::formatMoney(received) + ".");
            context.getIO()->showMessage("Uang kamu sekarang: " + TextFormatter::formatMoney(player.getBalance()));
        }
    }

    void mortgagePropertyForLiquidation(Player& player, PropertyTile* property, GameContext& context) {
        if (property == nullptr || property->getStatus() != PropertyStatus::OWNED) {
            return;
        }

        int received = mortgageValueForLiquidation(player, property);
        if (received <= 0) {
            return;
        }

        property->mortgage();
        player += received;
        context.logEvent(
            "LIKUIDASI",
            player.getUsername() + " menggadaikan " + property->getCode() +
            " dan menerima " + TextFormatter::formatMoney(received));
        if (context.getIO() != nullptr) {
            context.getIO()->showPaymentNotification(
                "LIQUIDATION",
                player.getUsername() + " menggadaikan " + property->getName() +
                " dan menerima " + TextFormatter::formatMoney(received) + ".");
            context.getIO()->showMessage(
                property->getName() + " berhasil digadaikan. Kamu menerima " + TextFormatter::formatMoney(received) + ".");
            context.getIO()->showMessage("Uang kamu sekarang: " + TextFormatter::formatMoney(player.getBalance()));
        }
    }

    bool autoLiquidateAssets(Player& player, int amount, GameContext& context) {
        while (player.getBalance() < amount) {
            std::vector<LiquidationOption> options = buildLiquidationOptions(player);
            if (options.empty()) {
                return false;
            }

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
        }

        return true;
    }

    bool promptAndExecuteLiquidationPlan(Player& player, int amount, GameContext& context, GameIO& io) {
        std::vector<LiquidationCandidate> candidates = buildLiquidationCandidates(player);
        if (candidates.empty()) {
            return false;
        }

        std::vector<LiquidationDecision> decisions;
        if (!io.promptLiquidationPlan(player, amount, candidates, decisions)) {
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
}

bool BankruptcyHandler::handleBankruptcy(Player& player, Player* creditor, int amount, GameContext& context) {
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
            io->showMessage("Tidak cukup untuk menutup kewajiban " + TextFormatter::formatMoney(amount) + ".");
            io->showMessage("");
            io->showMessage(player.getUsername() + " dinyatakan BANGKRUT!");
            io->showMessage("Kreditor: " + std::string(creditor == nullptr ? "Bank" : creditor->getUsername()));
            io->showMessage("");
        }
        if (creditor) {
            if (io != nullptr) {
                io->showMessage("Pengalihan aset ke " + creditor->getUsername() + ":");
                io->showMessage("  - Uang tunai sisa  : " + TextFormatter::formatMoney(player.getBalance()));
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
                io->showMessage("Uang sisa " + TextFormatter::formatMoney(player.getBalance()) + " diserahkan ke Bank.");
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
    }

    if (io != nullptr) {
        io->showMessage("Dana likuidasi dapat menutup kewajiban.");
        io->showMessage("Kamu wajib melikuidasi aset untuk membayar.");
        io->showMessage("");
    }

    const bool liquidationCompleted = liquidateAssets(player, amount, context);
    if (!liquidationCompleted) {
        if (io != nullptr) {
            io->showMessage("Likuidasi aset dibatalkan atau dana belum cukup untuk menutup kewajiban.");
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
        io->showMessage("Kewajiban " + TextFormatter::formatMoney(amount) + " terpenuhi. Membayar ke " + creditor->getUsername() + "...");
        io->showMessage(
            "Uang kamu : " + TextFormatter::formatMoney(player.getBalance()) + " -> " + TextFormatter::formatMoney(player.getBalance() - amount)
        );
        io->showMessage(
            "Uang " + creditor->getUsername() + ": " + TextFormatter::formatMoney(creditor->getBalance()) +
            " -> " + TextFormatter::formatMoney(creditor->getBalance() + amount)
        );
    }
    player.setBalance(player.getBalance() - amount);
    if (creditor) {
        creditor->setBalance(creditor->getBalance() + amount);
    }
    if (context.getIO() != nullptr) {
        context.getIO()->showPaymentNotification(
            "PAYMENT",
            player.getUsername() + " membayar " + TextFormatter::formatMoney(amount) +
                (creditor == nullptr ? " ke Bank." : " kepada " + creditor->getUsername() + "."));
    }
    return true;
}

int BankruptcyHandler::calculateLiquidationMax(Player& player) const {
    return player.getLiquidationMax();
}

bool BankruptcyHandler::liquidateAssets(Player& player, int amount, GameContext& context) {
    if (player.getBalance() >= amount) {
        return true;
    }

    GameIO* io = context.getIO();
    if (io == nullptr) {
        return autoLiquidateAssets(player, amount, context);
    }

    return promptAndExecuteLiquidationPlan(player, amount, context, *io);
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
                    Player* highestBidder = auction->getHighestBidder();
                    amount = io->promptAuctionBid(
                        *property,
                        *bidder,
                        auction->getHighestBid(),
                        highestBidder == nullptr ? "" : highestBidder->getUsername());
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
                            "Penawaran tertinggi: " + TextFormatter::formatMoney(auction->getHighestBid()) +
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

                if (io != nullptr) {
                    io->showMessage(
                        "Belum ada pemain yang melakukan bid. " +
                        forcedBidder->getUsername() + " otomatis melakukan bid awal M0.");
                }

                try {
                    auction->processBid(forcedBidder, 0);
                    if (io != nullptr) {
                        io->showMessage(
                            "Penawaran tertinggi: " + TextFormatter::formatMoney(auction->getHighestBid()) +
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
                " seharga " + TextFormatter::formatMoney(winningBid));
            if (io != nullptr) {
                io->showMessage("Lelang selesai!");
                io->showMessage("Pemenang: " + winner->getUsername());
                io->showMessage("Harga akhir: " + TextFormatter::formatMoney(winningBid));
                io->showMessage("");
                io->showPaymentNotification(
                    "PAYMENT",
                    winner->getUsername() + " membayar " + TextFormatter::formatMoney(winningBid) +
                        " ke Bank untuk memenangkan " + property->getName() + ".");
                io->showAuctionNotification(
                    "AUCTION",
                    winner->getUsername() + " memenangkan " + property->getName() +
                        " seharga " + TextFormatter::formatMoney(winningBid) + ".");
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
