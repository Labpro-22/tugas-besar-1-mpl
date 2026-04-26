#include "core/GameIO.hpp"

#include <string>

#include "core/Board.hpp"
#include "core/DeckFactory.hpp"
#include "core/TurnManager.hpp"
#include "models/cards/ActionCard.hpp"
#include "models/cards/SkillCard.hpp"
#include "models/Enums.hpp"
#include "models/Player.hpp"
#include "models/state/LogEntry.hpp"
#include "models/tiles/PropertyTile.hpp"
#include "utils/TextFormatter.hpp"

void GameIO::showPawnStep(const Player&, int)
{
}

void GameIO::showDiceRoll(int, int)
{
}

void GameIO::showDiceLanding(int, int, int, const std::string&, const std::string&, const std::string&)
{
}

bool GameIO::confirmPropertyPurchase(const Player& player, const PropertyTile& property)
{
    const int originalPrice = property.getBuyPrice();
    const int finalPrice = player.getDiscountedAmount(originalPrice);

    if (!player.canAfford(finalPrice)) {
        return false;
    }

    std::string prompt = "Apakah kamu ingin membeli properti ini seharga " +
        TextFormatter::formatMoney(finalPrice);
    if (finalPrice != originalPrice) {
        prompt += " (setelah diskon dari " + TextFormatter::formatMoney(originalPrice) + ")";
    }
    prompt += "?";

    return confirmYN(prompt);
}

void GameIO::showPropertyNotice(const Player&, const PropertyTile& property)
{
    showMessage(property.getName());
}

std::string GameIO::promptText(const std::string&)
{
    return "";
}

void GameIO::showActionCard(CardType, const ActionCard& card)
{
    showMessage("Kartu: \"" + card.getText() + "\"");
}

void GameIO::showPaymentNotification(const std::string& title, const std::string& detail)
{
    showMessage(title + ": " + detail);
}

void GameIO::showAuctionNotification(const std::string& title, const std::string& detail)
{
    showMessage(title + ": " + detail);
}

void GameIO::showStreetPurchasePreview(
    const Player& player,
    const PropertyTile&,
    const StreetTile&,
    int originalPrice,
    int finalPrice
)
{
    showMessage("Harga beli: " + TextFormatter::formatMoney(originalPrice));
    showMessage("Uang kamu saat ini: " + TextFormatter::formatMoney(player.getBalance()));
    if (finalPrice != originalPrice) {
        showMessage(
            "Diskon aktif. Harga beli menjadi " + TextFormatter::formatMoney(finalPrice) +
            " dari " + TextFormatter::formatMoney(originalPrice) + "."
        );
    }
}

bool GameIO::usesRichGuiPresentation() const
{
    return false;
}

int GameIO::promptAuctionBid(const PropertyTile&, const Player& bidder, int highestBid)
{
    return promptInt(
        bidder.getUsername() +
            " saldo " + TextFormatter::formatMoney(bidder.getBalance()) +
            ", bid tertinggi " + TextFormatter::formatMoney(highestBid) +
            ". Masukkan bid (0 untuk pass): ");
}

int GameIO::promptAuctionBid(
    const PropertyTile& property,
    const Player& bidder,
    int highestBid,
    const std::string&
)
{
    return promptAuctionBid(property, bidder, highestBid);
}

int GameIO::promptTaxPaymentOption(
    const Player&,
    const std::string& tileName,
    int flatAmount,
    int percentage,
    int wealth,
    int percentageAmount
)
{
    showMessage("Pilih opsi pembayaran " + tileName + ":");
    showMessage("Bayar tetap: " + TextFormatter::formatMoney(flatAmount));
    showMessage(
        "Bayar berdasarkan kekayaan: " + std::to_string(percentage) +
            "% dari total kekayaan " + TextFormatter::formatMoney(wealth) +
            " = " + TextFormatter::formatMoney(percentageAmount));

    const int choice = promptIntInRange(
        "Pilih pembayaran: 1 untuk bayar tetap, 2 untuk persentase kekayaan: ",
        1,
        2);
    return choice;
}

bool GameIO::promptLiquidationPlan(
    const Player& player,
    int targetAmount,
    const std::vector<LiquidationCandidate>& candidates,
    std::vector<LiquidationDecision>& decisions
)
{
    decisions.clear();
    if (player.getBalance() >= targetAmount) {
        return true;
    }

    auto candidateForTile = [&](int tileIndex) -> const LiquidationCandidate* {
        for (const LiquidationCandidate& candidate : candidates) {
            if (candidate.tileIndex == tileIndex) {
                return &candidate;
            }
        }
        return nullptr;
    };

    auto isPlanned = [&](int tileIndex) {
        for (const LiquidationDecision& decision : decisions) {
            if (decision.tileIndex == tileIndex) {
                return true;
            }
        }
        return false;
    };

    auto plannedGain = [&]() {
        int total = 0;
        for (const LiquidationDecision& decision : decisions) {
            const LiquidationCandidate* candidate = candidateForTile(decision.tileIndex);
            if (candidate == nullptr) {
                continue;
            }

            total += decision.action == LiquidationActionKind::Sell
                ? candidate->sellValue
                : candidate->mortgageValue;
        }
        return total;
    };

    auto removePlannedAt = [&](int index) {
        if (index < 0 || index >= static_cast<int>(decisions.size())) {
            return;
        }
        decisions.erase(decisions.begin() + index);
    };

    while (true) {
        const int gain = plannedGain();
        const int shortage = targetAmount - (player.getBalance() + gain);

        showMessage("");
        showMessage("=== Rencana Likuidasi Aset ===");
        showMessage("Uang saat ini      : " + TextFormatter::formatMoney(player.getBalance()));
        showMessage("Target kewajiban   : " + TextFormatter::formatMoney(targetAmount));
        showMessage("Dana rencana       : " + TextFormatter::formatMoney(gain));
        if (shortage > 0) {
            showMessage("Sisa kekurangan    : " + TextFormatter::formatMoney(shortage));
        } else {
            showMessage("Target terpenuhi   : surplus " + TextFormatter::formatMoney(-shortage));
        }

        if (decisions.empty()) {
            showMessage("Rencana terpilih   : belum ada");
        } else {
            showMessage("Rencana terpilih:");
            for (int i = 0; i < static_cast<int>(decisions.size()); ++i) {
                const LiquidationDecision& decision = decisions[i];
                const LiquidationCandidate* candidate = candidateForTile(decision.tileIndex);
                if (candidate == nullptr) {
                    continue;
                }

                const bool isSell = decision.action == LiquidationActionKind::Sell;
                const int value = isSell ? candidate->sellValue : candidate->mortgageValue;
                showMessage(
                    std::to_string(i + 1) + ". " +
                    (isSell ? "Jual ke Bank " : "Gadai ") +
                    candidate->name + " (" + candidate->code + ") - " +
                    TextFormatter::formatMoney(value));
            }
        }

        std::vector<LiquidationDecision> availableActions;
        showMessage("");
        showMessage("Opsi aset yang belum dipilih:");
        for (const LiquidationCandidate& candidate : candidates) {
            if (isPlanned(candidate.tileIndex)) {
                continue;
            }

            if (candidate.sellValue > 0) {
                availableActions.push_back({candidate.tileIndex, LiquidationActionKind::Sell});
                showMessage(
                    std::to_string(static_cast<int>(availableActions.size())) +
                    ". Jual ke Bank " + candidate.name +
                    " (" + candidate.code + ") - " + TextFormatter::formatMoney(candidate.sellValue));
            }
            if (candidate.mortgageValue > 0) {
                availableActions.push_back({candidate.tileIndex, LiquidationActionKind::Mortgage});
                showMessage(
                    std::to_string(static_cast<int>(availableActions.size())) +
                    ". Gadai " + candidate.name +
                    " (" + candidate.code + ") - " + TextFormatter::formatMoney(candidate.mortgageValue));
            }
        }

        if (availableActions.empty()) {
            showMessage("Tidak ada opsi aset lain yang tersedia.");
        }

        showMessage("");
        showMessage("0. Batalkan likuidasi");
        const int undoMenu = static_cast<int>(availableActions.size()) + 1;
        const int clearMenu = static_cast<int>(availableActions.size()) + 2;
        const int finalizeMenu = static_cast<int>(availableActions.size()) + 3;
        showMessage(std::to_string(undoMenu) + ". Undo pilihan tertentu");
        showMessage(std::to_string(clearMenu) + ". Kosongkan rencana");
        showMessage(std::to_string(finalizeMenu) + ". Finalize rencana");

        const int choice = promptIntInRange(
            "Pilih aksi likuidasi: ",
            0,
            finalizeMenu);

        if (choice == 0) {
            decisions.clear();
            return false;
        }

        if (choice >= 1 && choice <= static_cast<int>(availableActions.size())) {
            decisions.push_back(availableActions[choice - 1]);
            continue;
        }

        if (choice == undoMenu) {
            if (decisions.empty()) {
                showMessage("Belum ada pilihan yang bisa di-undo.");
                continue;
            }

            const int undoChoice = promptIntInRange(
                "Pilih nomor rencana yang ingin dibatalkan (0 untuk kembali): ",
                0,
                static_cast<int>(decisions.size()));
            if (undoChoice > 0) {
                removePlannedAt(undoChoice - 1);
            }
            continue;
        }

        if (choice == clearMenu) {
            decisions.clear();
            continue;
        }

        if (choice == finalizeMenu) {
            if (decisions.empty()) {
                showMessage("Rencana masih kosong. Pilih aset terlebih dahulu.");
                continue;
            }
            if (targetAmount - (player.getBalance() + plannedGain()) > 0) {
                showMessage("Dana rencana belum cukup untuk menutup kewajiban. Pilih aset lagi.");
                continue;
            }

            if (confirmYN("Jalankan rencana likuidasi ini?")) {
                return true;
            }
        }
    }
}

int GameIO::promptTileSelection(const std::string& title, const std::vector<int>& validTileIndices)
{
    return promptTileSelection(title, validTileIndices, false);
}

int GameIO::promptTileSelection(
    const std::string& title,
    const std::vector<int>& validTileIndices,
    bool allowCancel
)
{
    showMessage(title);
    for (int option = 0; option < static_cast<int>(validTileIndices.size()); ++option) {
        showMessage(
            std::to_string(option + 1) + ". Tile " +
            std::to_string(validTileIndices[static_cast<std::size_t>(option)] + 1));
    }

    if (allowCancel) {
        showMessage("0. Batal");
        const int choice = promptIntInRange(
            "Pilih tile (0-" + std::to_string(validTileIndices.size()) + "): ",
            0,
            static_cast<int>(validTileIndices.size()));
        if (choice == 0) {
            return -1;
        }
        return validTileIndices[static_cast<std::size_t>(choice - 1)];
    }

    const int choice = promptIntInRange(
        "Pilih tile (1-" + std::to_string(validTileIndices.size()) + "): ",
        1,
        static_cast<int>(validTileIndices.size()));

    return validTileIndices[static_cast<std::size_t>(choice - 1)];
}

int GameIO::promptSkillCardSelection(
    const std::string& title,
    const std::vector<SkillCard*>& cards,
    bool allowCancel
)
{
    showMessage(title);
    for (int i = 0; i < static_cast<int>(cards.size()); ++i) {
        if (cards[i] == nullptr) {
            continue;
        }
        showMessage(
            std::to_string(i + 1) + ". " + cards[i]->getTypeName() +
                " - " + DeckFactory::describeSkillCard(cards[i]));
    }
    if (allowCancel) {
        showMessage("0. Batal");
        return promptIntInRange(
            "Pilih kartu yang ingin digunakan (0-" + std::to_string(cards.size()) + "): ",
            0,
            static_cast<int>(cards.size()));
    }

    return promptIntInRange(
        "Pilih nomor kartu yang ingin dibuang (1-" + std::to_string(cards.size()) + "): ",
        1,
        static_cast<int>(cards.size()));
}

void GameIO::showHelp(const Player&)
{
}

void GameIO::renderBoard(const Board&, const std::vector<Player>&, const TurnManager&)
{
}

void GameIO::showPropertyDeed(const PropertyTile*)
{
}

void GameIO::showPlayerProperties(const Player&)
{
}

void GameIO::showLogEntries(const std::vector<LogEntry>&)
{
}
