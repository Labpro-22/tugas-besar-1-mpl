#include "core/PropertyTransactionService.hpp"

#include <exception>
#include <string>
#include <vector>

#include "core/AuctionManager.hpp"
#include "core/BankruptcyHandler.hpp"
#include "core/Dice.hpp"
#include "core/GameContext.hpp"
#include "core/GameIO.hpp"
#include "core/TurnManager.hpp"
#include "models/Player.hpp"
#include "models/tiles/PropertyTile.hpp"
#include "models/tiles/StreetTile.hpp"
#include "utils/OutputFormatter.hpp"
#include "utils/TransactionLogger.hpp"

namespace {
    int getCurrentTurn(const GameContext& context) {
        TurnManager* turnManager = context.getTurnManager();
        return turnManager == nullptr ? 0 : turnManager->getCurrentTurn();
    }

    std::string getRentConditionLabel(const PropertyTile& tile) {
        const StreetTile* street = tile.asStreetTile();
        if (street != nullptr) {
            int level = street->getBuildingLevel();
            if (level <= 0) {
                return "Tanpa bangunan";
            }
            return OutputFormatter::formatBuildingLevel(level);
        }

        if (tile.getPropertyType() == PropertyType::RAILROAD) {
            return "Stasiun";
        }

        if (tile.getPropertyType() == PropertyType::UTILITY) {
            return "Utilitas";
        }

        return "-";
    }
}

void PropertyTransactionService::handlePropertyLanded(
    Player& player,
    PropertyTile& tile,
    GameContext& context
) {
    GameIO* io = context.getIO();

    if (tile.getStatus() == PropertyStatus::BANK) {
        int price = tile.getBuyPrice();
        StreetTile* street = tile.asStreetTile();

        if (io != nullptr) {
            io->showMessage(
                "Kamu mendarat di " + tile.getName() + " (" + tile.getCode() + ")!");
        }

        if (street != nullptr) {
            int priceToPay = player.getDiscountedAmount(price);
            if (io != nullptr) {
                for (const std::string& line : OutputFormatter::formatStreetPurchasePreview(tile, *street)) {
                    io->showMessage(line);
                }
                io->showMessage("Uang kamu saat ini: " + OutputFormatter::formatMoney(player.getBalance()));
                if (priceToPay != price) {
                    io->showMessage(
                        "Diskon aktif. Harga beli menjadi " + OutputFormatter::formatMoney(priceToPay) +
                        " dari " + OutputFormatter::formatMoney(price) + "."
                    );
                }
            }

            if (price > 0 && io != nullptr && io->confirmPropertyPurchase(player, tile)) {
                int finalPrice = player.consumeDiscountedAmount(price);
                player -= finalPrice;
                tile.transferTo(player);

                io->showMessage(tile.getName() + " kini menjadi milikmu!");
                io->showMessage("Uang tersisa: " + OutputFormatter::formatMoney(player.getBalance()));
                context.logEvent(
                    "BELI",
                    "Beli " + tile.getName() + " (" + tile.getCode() +
                        ") seharga " + OutputFormatter::formatMoney(finalPrice));
                return;
            }

            if (io != nullptr) {
                io->showMessage("Properti ini akan masuk ke sistem lelang...");
            }
        } else if (price <= 0 || player.canAfford(player.getDiscountedAmount(price))) {
            int finalPrice = player.consumeDiscountedAmount(price);
            if (finalPrice > 0) {
                player -= finalPrice;
            }
            tile.transferTo(player);
            if (io != nullptr) {
                io->showMessage(
                    "Belum ada yang menginjaknya duluan, " + tile.getName() +
                        " kini menjadi milikmu!");
            }
            context.logEvent(
                "BELI",
                tile.getName() + " (" + tile.getCode() + ") kini milik " +
                    player.getUsername() + " (otomatis)");
            return;
        } else if (io != nullptr) {
            io->showMessage("Uang tidak cukup. Properti ini akan masuk ke sistem lelang...");
        }

        AuctionManager* auction = context.getAuctionManager();
        TurnManager* turnManager = context.getTurnManager();
        if (auction == nullptr || turnManager == nullptr) {
            context.logEvent(
                "LELANG",
                tile.getName() + " belum terbeli karena saldo tidak cukup.");
            return;
        }

        std::vector<Player*> activePlayers = turnManager->getActivePlayers();
        auction->conductAuction(&tile, activePlayers, &player);

        std::vector<Player*> auctionOrder = auction->getAuctionOrder();
        int totalPlayers = static_cast<int>(auctionOrder.size());

        if (io != nullptr) {
            io->showMessage("Properti " + tile.getName() + " (" + tile.getCode() + ") akan dilelang!");
            if (!auctionOrder.empty()) {
                io->showMessage("");
                io->showMessage(
                    "Urutan lelang dimulai dari pemain setelah " + player.getUsername() + "."
                );
                io->showMessage("");
            }
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
                    if (!auction->processBid(bidder, amount)) {
                        if (io != nullptr) {
                            io->showMessage("Bid harus lebih besar dari bid tertinggi.");
                        }
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
            if (io != nullptr) {
                io->showMessage("Lelang selesai!");
                io->showMessage("Pemenang: " + winner->getUsername());
                io->showMessage("Harga akhir: " + OutputFormatter::formatMoney(winningBid));
                io->showMessage("");
                io->showMessage(
                    "Properti " + tile.getName() + " (" + tile.getCode() +
                    ") kini dimiliki " + winner->getUsername() + "."
                );
            }
            context.logEvent(
                "LELANG",
                "Menang lelang " + tile.getName() + " (" + tile.getCode() +
                    ") seharga " + OutputFormatter::formatMoney(winningBid));
        } else {
            if (io != nullptr) {
                io->showMessage("Lelang selesai! " + tile.getName() + " tidak terjual.");
            }
            context.logEvent("LELANG", tile.getName() + " tidak terjual.");
        }

        return;
    }

    if (tile.getStatus() == PropertyStatus::OWNED && !tile.isOwnedBy(player)) {
        handleRentPayment(player, tile, context);
    }
}

void PropertyTransactionService::handleRentPayment(
    Player& player,
    PropertyTile& tile,
    GameContext& context
) {
    GameIO* io = context.getIO();
    TurnManager* turnManager = context.getTurnManager();
    Player* currentPlayer = turnManager == nullptr ? nullptr : turnManager->getCurrentPlayer();
    bool payerIsCurrentPlayer = currentPlayer == nullptr || currentPlayer == &player;
    const std::string payerLabel = payerIsCurrentPlayer ? "Kamu" : player.getUsername();
    const std::string payerBalanceLabel = payerIsCurrentPlayer ? "Uang kamu" : "Uang " + player.getUsername();

    if (tile.isMortgaged() || tile.isOwnedBy(player)) {
        if (tile.isMortgaged() && io != nullptr) {
            io->showMessage(
                payerLabel + " mendarat di " + tile.getName() + " (" + tile.getCode() +
                "), milik " + tile.getOwner()->getUsername() + ".");
            io->showMessage("Properti ini sedang digadaikan [M]. Tidak ada sewa yang dikenakan.");
        }
        return;
    }

    if (player.isShieldActive()) {
        if (io != nullptr) {
            io->showMessage("[SHIELD ACTIVE]: Efek ShieldCard melindungi Anda dari tagihan sewa.");
        }
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
    if (rentAmount <= 0) {
        return;
    }

    Player* owner = tile.getOwner();
    if (owner == nullptr) {
        return;
    }

    int originalRentAmount = rentAmount;
    rentAmount = player.consumeDiscountedAmount(rentAmount);

    if (io != nullptr) {
        io->showMessage(
            payerLabel + " mendarat di " + tile.getName() + " (" + tile.getCode() +
                "), milik " + owner->getUsername() + "!");
        io->showMessage("");
        io->showMessage("Kondisi      : " + getRentConditionLabel(tile));
        io->showMessage("Sewa         : " + OutputFormatter::formatMoney(rentAmount));
        if (tile.getFestivalDuration() > 0) {
            io->showMessage(
                "Festival     : aktif x" + std::to_string(tile.getFestivalMultiplier()) +
                ", sisa " + std::to_string(tile.getFestivalDuration()) + " giliran"
            );
        }
        if (rentAmount != originalRentAmount) {
            io->showMessage(
                "Diskon aktif  : " + OutputFormatter::formatMoney(originalRentAmount) +
                " -> " + OutputFormatter::formatMoney(rentAmount)
            );
        }
        io->showMessage("");
    }

    if (!player.canAfford(rentAmount)) {
        if (io != nullptr) {
            io->showMessage(
                payerLabel + " tidak mampu membayar sewa penuh! (" +
                    OutputFormatter::formatMoney(rentAmount) + ")");
            io->showMessage(payerBalanceLabel + " saat ini: " + OutputFormatter::formatMoney(player.getBalance()));
        }
        BankruptcyHandler* bankruptcyHandler = context.getBankruptcyHandler();
        if (bankruptcyHandler != nullptr) {
            bankruptcyHandler->handleBankruptcy(player, owner, rentAmount, context);
        } else {
            player -= rentAmount;
            *owner += rentAmount;
        }
        return;
    }

    int playerBefore = player.getBalance();
    int ownerBefore = owner->getBalance();
    player -= rentAmount;
    *owner += rentAmount;

    if (io != nullptr) {
        io->showMessage(
            payerBalanceLabel + "     : " + OutputFormatter::formatMoney(playerBefore) +
                " -> " + OutputFormatter::formatMoney(player.getBalance()));
        io->showMessage(
            "Uang " + owner->getUsername() + " : " + OutputFormatter::formatMoney(ownerBefore) +
                " -> " + OutputFormatter::formatMoney(owner->getBalance()));
    }

    TransactionLogger* logger = context.getLogger();
    if (logger != nullptr) {
        std::string detail =
            "Bayar " + OutputFormatter::formatMoney(rentAmount) + " ke " + owner->getUsername() +
            " (" + tile.getCode() + ", " + getRentConditionLabel(tile);
        if (tile.getFestivalDuration() > 0) {
            detail += ", festival aktif x" + std::to_string(tile.getFestivalMultiplier());
        }
        detail += ")";

        logger->log(
            getCurrentTurn(context),
            player.getUsername(),
            "SEWA",
            detail);
    }
}
