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
#include "utils/TransactionLogger.hpp"

namespace {
    int getCurrentTurn(const GameContext& context) {
        TurnManager* turnManager = context.getTurnManager();
        return turnManager == nullptr ? 0 : turnManager->getCurrentTurn();
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
                io->showMessage("Harga beli : M" + std::to_string(price));
                if (priceToPay != price) {
                    io->showMessage("Harga setelah diskon : M" + std::to_string(priceToPay));
                }
                io->showMessage("Sewa dasar : M" + std::to_string(street->getRentAtLevel(0)));
                io->showMessage("Uang kamu saat ini: M" + std::to_string(player.getBalance()));
            }

            if (price > 0 && io != nullptr &&
                io->confirmPropertyPurchase(player, tile)) {
                int beforeBalance = player.getBalance();
                int finalPrice = player.consumeDiscountedAmount(price);
                player -= finalPrice;
                tile.transferTo(player);

                io->showPaymentNotification(
                    "PAYMENT",
                    player.getUsername() + " membayar M" + std::to_string(price) +
                        " ke Bank untuk membeli " + tile.getName() + ".");
                io->showMessage(tile.getName() + " kini menjadi milikmu!");
                io->showMessage(
                    "Uang kamu: M" + std::to_string(beforeBalance) +
                        " -> M" + std::to_string(player.getBalance()));
                context.logEvent(
                    "BELI",
                    player.getUsername() + " membeli " + tile.getName() +
                        " seharga M" + std::to_string(finalPrice));
                return;
            }

            if (io != nullptr) {
                if (price > 0 && !player.canAfford(priceToPay)) {
                    io->showMessage("Uang tidak cukup untuk membeli properti ini.");
                }
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
                player.getUsername() + " mendapatkan " + tile.getName() +
                    " seharga M" + std::to_string(finalPrice) + ".");
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
                    if (!auction->processBid(bidder, amount)) {
                        if (io != nullptr) {
                            io->showMessage("Bid harus lebih besar dari bid tertinggi.");
                        }
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
            if (io != nullptr) {
                io->showPaymentNotification(
                    "PAYMENT",
                    winner->getUsername() + " membayar M" + std::to_string(winningBid) +
                        " ke Bank untuk memenangkan " + tile.getName() + ".");
                io->showAuctionNotification(
                    "AUCTION",
                    winner->getUsername() + " memenangkan " + tile.getName() +
                        " seharga M" + std::to_string(winningBid) + ".");
                io->showMessage(
                    "Lelang selesai! " + winner->getUsername() +
                        " memenangkan " + tile.getName() +
                        " seharga M" + std::to_string(winningBid) + ".");
            }
            context.logEvent(
                "LELANG",
                winner->getUsername() + " memenangkan " + tile.getName() +
                    " seharga M" + std::to_string(winningBid));
        } else {
            if (io != nullptr) {
                io->showAuctionNotification(
                    "AUCTION",
                    "Lelang selesai. " + tile.getName() + " tidak terjual.");
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
                payerLabel + " mendarat di " + tile.getName() + " (" + tile.getCode() + ").");
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
        io->showMessage("Sewa: M" + std::to_string(rentAmount));
        if (rentAmount != originalRentAmount) {
            io->showMessage("Diskon diterapkan dari M" + std::to_string(originalRentAmount) +
                " menjadi M" + std::to_string(rentAmount) + ".");
        }
    }

    if (!player.canAfford(rentAmount)) {
        if (io != nullptr) {
            io->showMessage(
                payerLabel + " tidak mampu membayar sewa penuh! (M" +
                    std::to_string(rentAmount) + ")");
            io->showMessage(payerBalanceLabel + " saat ini: M" + std::to_string(player.getBalance()));
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
        io->showPaymentNotification(
            "PAYMENT",
            player.getUsername() + " membayar sewa M" + std::to_string(rentAmount) +
                " kepada " + owner->getUsername() + ".");
        io->showMessage(
            payerBalanceLabel + ": M" + std::to_string(playerBefore) +
                " -> M" + std::to_string(player.getBalance()));
        io->showMessage(
            "Uang " + owner->getUsername() + ": M" + std::to_string(ownerBefore) +
                " -> M" + std::to_string(owner->getBalance()));
    }

    TransactionLogger* logger = context.getLogger();
    if (logger != nullptr) {
        logger->log(
            getCurrentTurn(context),
            player.getUsername(),
            "SEWA",
            player.getUsername() + " membayar sewa ke " + owner->getUsername() +
                " sebesar M" + std::to_string(rentAmount));
    }
}
