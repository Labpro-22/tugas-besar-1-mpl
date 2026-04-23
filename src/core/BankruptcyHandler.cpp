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
        if (io != nullptr && context.getTurnManager() != nullptr) {
            io->showMessage(player.getUsername() + " telah keluar dari permainan.");
            io->showMessage(
                "Permainan berlanjut dengan " +
                std::to_string(context.getTurnManager()->getActivePlayers().size()) +
                " pemain tersisa."
            );
        }
    } else {
        if (io != nullptr) {
            io->showMessage("Dana likuidasi dapat menutup kewajiban.");
            io->showMessage("Kamu wajib melikuidasi aset untuk membayar.");
            io->showMessage("");
        }
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

        showLiquidationPanel(player, amount, options, context);

        int choice = io->promptIntInRange(
            "Pilih aksi (0 jika sudah cukup): ",
            0,
            static_cast<int>(options.size()));
        if (choice == 0) {
            if (player.getBalance() >= amount) {
                return true;
            }
            io->showMessage("Dana belum cukup untuk menutup kewajiban.");
            continue;
        }

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
                    amount = io->promptAuctionBid(
                        bidder->getUsername(),
                        auction->getHighestBid(),
                        bidder->getBalance());
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
                    amount = io->promptAuctionBid(
                        forcedBidder->getUsername(),
                        auction->getHighestBid(),
                        forcedBidder->getBalance());
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
                io->showMessage(
                    "Properti " + property->getName() + " (" + property->getCode() +
                    ") kini dimiliki " + winner->getUsername() + "."
                );
            }
        } else {
            context.logEvent("LELANG", property->getCode() + " tidak terjual pada lelang aset bangkrut.");
            if (io != nullptr) {
                io->showMessage("Lelang selesai!");
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
