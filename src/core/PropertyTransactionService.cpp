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
#include "utils/TextFormatter.hpp"
#include "utils/TransactionLogger.hpp"

namespace {
    int getCurrentTurn(const GameContext& context) {
        TurnManager* turnManager = context.getTurnManager();
        return turnManager == nullptr ? 0 : turnManager->getCurrentTurn();
    }

    void logAuctionEvent(const GameContext& context, const std::string& username, const std::string& detail) {
        TransactionLogger* logger = context.getLogger();
        if (logger != nullptr) {
            logger->log(getCurrentTurn(context), username, "LELANG", detail);
        }
    }

    std::string getRentConditionLabel(const PropertyTile& tile) {
        const StreetTile* street = tile.asStreetTile();
        if (street != nullptr) {
            int level = street->getBuildingLevel();
            if (level <= 0) {
                return "Tanpa bangunan";
            }
            return TextFormatter::formatBuildingLevel(level);
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
                io->showStreetPurchasePreview(player, tile, *street, price, priceToPay);
            }

            if (price > 0 && io != nullptr && io->confirmPropertyPurchase(player, tile)) {
                int finalPrice = player.consumeDiscountedAmount(price);
                player -= finalPrice;
                tile.transferTo(player);

                io->showPaymentNotification(
                    "PAYMENT",
                    player.getUsername() + " membayar " + TextFormatter::formatMoney(finalPrice) +
                        " ke Bank untuk membeli " + tile.getName() + ".");
                io->showMessage(tile.getName() + " kini menjadi milikmu!");
                io->showMessage("Uang tersisa: " + TextFormatter::formatMoney(player.getBalance()));
                context.logEvent(
                    "BELI",
                    "Beli " + tile.getName() + " (" + tile.getCode() +
                        ") seharga " + TextFormatter::formatMoney(finalPrice));
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
            if (io != nullptr) {
                io->showPropertyNotice(player, tile);
                if (finalPrice > 0) {
                    io->showPaymentNotification(
                        "PAYMENT",
                        player.getUsername() + " membayar " + TextFormatter::formatMoney(finalPrice) +
                            " ke Bank untuk mendapatkan " + tile.getName() + ".");
                }
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
                    player.getUsername() + " (otomatis, harga " +
                    TextFormatter::formatMoney(finalPrice) + ")");
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
            io->showAuctionNotification(
                "AUCTION",
                "Lelang " + tile.getName() + " (" + tile.getCode() + ") dimulai.");
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

                bool bidResolved = false;
                while (!bidResolved) {
                    int amount = 0;
                    if (io != nullptr) {
                        Player* highestBidder = auction->getHighestBidder();
                        amount = io->promptAuctionBid(
                            tile,
                            *bidder,
                            auction->getHighestBid(),
                            highestBidder == nullptr ? "" : highestBidder->getUsername());
                    }

                    if (amount < 0) {
                        auction->processPass(bidder);
                        logAuctionEvent(
                            context,
                            bidder->getUsername(),
                            "Pass lelang " + tile.getName() + " (" + tile.getCode() + ")");
                        bidResolved = true;
                        continue;
                    }

                    try {
                        if (!auction->processBid(bidder, amount)) {
                            if (io != nullptr) {
                                io->showMessage("Bid harus lebih besar dari bid tertinggi.");
                            }
                            bidResolved = io == nullptr;
                        } else {
                            if (io != nullptr) {
                                io->showMessage(
                                    "Penawaran tertinggi: " + TextFormatter::formatMoney(auction->getHighestBid()) +
                                        " (" + bidder->getUsername() + ")"
                                );
                                io->showMessage("");
                            }
                            logAuctionEvent(
                                context,
                                bidder->getUsername(),
                                "Bid " + TextFormatter::formatMoney(amount) + " untuk " +
                                    tile.getName() + " (" + tile.getCode() + ")");
                            bidResolved = true;
                        }
                    } catch (const std::exception& e) {
                        if (io != nullptr) {
                            io->showError(e, context.getLogger(), getCurrentTurn(context), bidder->getUsername());
                        }
                        bidResolved = io == nullptr;
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
                    logAuctionEvent(
                        context,
                        forcedBidder->getUsername(),
                        "Bid otomatis " + TextFormatter::formatMoney(0) + " untuk " +
                            tile.getName() + " (" + tile.getCode() + ")");
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
            if (io != nullptr) {
                io->showMessage("Lelang selesai!");
                io->showMessage("Pemenang: " + winner->getUsername());
                io->showMessage("Harga akhir: " + TextFormatter::formatMoney(winningBid));
                io->showMessage("");
                io->showPaymentNotification(
                    "PAYMENT",
                    winner->getUsername() + " membayar " + TextFormatter::formatMoney(winningBid) +
                        " ke Bank untuk memenangkan " + tile.getName() + ".");
                io->showAuctionNotification(
                    "AUCTION",
                    winner->getUsername() + " memenangkan " + tile.getName() +
                        " seharga " + TextFormatter::formatMoney(winningBid) + ".");
                io->showMessage(
                    "Properti " + tile.getName() + " (" + tile.getCode() +
                    ") kini dimiliki " + winner->getUsername() + "."
                );
            }
            logAuctionEvent(
                context,
                winner->getUsername(),
                "Menang lelang " + tile.getName() + " (" + tile.getCode() +
                    ") seharga " + TextFormatter::formatMoney(winningBid));
        } else {
            if (io != nullptr) {
                io->showAuctionNotification(
                    "AUCTION",
                    "Lelang selesai. " + tile.getName() + " tidak terjual.");
                io->showMessage("Lelang selesai! " + tile.getName() + " tidak terjual.");
            }
            logAuctionEvent(context, "SYSTEM", tile.getName() + " tidak terjual.");
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

    if (player.consumeShield()) {
        if (io != nullptr) {
            io->showMessage("[SHIELD ACTIVE]: Alhamdulillah, ShieldCard melindungimu dari tagihan sewa!");
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
        io->showMessage("Sewa         : " + TextFormatter::formatMoney(rentAmount));
        if (tile.getFestivalDuration() > 0) {
            io->showMessage(
                "Festival     : aktif x" + std::to_string(tile.getFestivalMultiplier()) +
                ", sisa " + std::to_string(tile.getFestivalDuration()) + " giliran"
            );
        }
        if (rentAmount != originalRentAmount) {
            io->showMessage(
                "Diskon aktif  : " + TextFormatter::formatMoney(originalRentAmount) +
                " -> " + TextFormatter::formatMoney(rentAmount)
            );
        }
        io->showMessage("");
    }

    if (!player.canAfford(rentAmount)) {
        if (io != nullptr) {
            io->showMessage(
                payerLabel + " tidak mampu membayar sewa penuh! (" +
                    TextFormatter::formatMoney(rentAmount) + ")");
            io->showMessage(payerBalanceLabel + " saat ini: " + TextFormatter::formatMoney(player.getBalance()));
        }
        BankruptcyHandler* bankruptcyHandler = context.getBankruptcyHandler();
        if (bankruptcyHandler != nullptr) {
            const bool settled = bankruptcyHandler->handleBankruptcy(player, owner, rentAmount, context);
            if (!settled && io != nullptr) {
                io->showMessage("Likuidasi dibatalkan. Pembayaran sewa belum dilakukan.");
            }
            if (settled && !player.isBankrupt() && context.getLogger() != nullptr) {
                context.getLogger()->log(
                    getCurrentTurn(context),
                    player.getUsername(),
                    "SEWA",
                    "Bayar sewa " + TextFormatter::formatMoney(rentAmount) + " ke " +
                        owner->getUsername() + " (" + tile.getCode() + ") setelah likuidasi.");
            }
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
        io->showPaymentNotification(
            "PAYMENT",
            player.getUsername() + " membayar sewa " + TextFormatter::formatMoney(rentAmount) +
                " kepada " + owner->getUsername() + ".");
        io->showMessage(
            payerBalanceLabel + "     : " + TextFormatter::formatMoney(playerBefore) +
                " -> " + TextFormatter::formatMoney(player.getBalance()));
        io->showMessage(
            "Uang " + owner->getUsername() + " : " + TextFormatter::formatMoney(ownerBefore) +
                " -> " + TextFormatter::formatMoney(owner->getBalance()));
    }

    TransactionLogger* logger = context.getLogger();
    if (logger != nullptr) {
        std::string detail =
            "Bayar " + TextFormatter::formatMoney(rentAmount) + " ke " + owner->getUsername() +
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
