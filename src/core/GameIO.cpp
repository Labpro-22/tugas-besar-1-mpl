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

int GameIO::promptAuctionBid(const std::string&, int, int)
{
    return -1;
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
            " saldo M" + std::to_string(bidder.getBalance()) +
            ", bid tertinggi M" + std::to_string(highestBid) +
            ". Masukkan bid (0 untuk pass): ");
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
    showMessage("Bayar tetap: M" + std::to_string(flatAmount));
    showMessage(
        "Bayar berdasarkan kekayaan: " + std::to_string(percentage) +
            "% dari total kekayaan M" + std::to_string(wealth) +
            " = M" + std::to_string(percentageAmount));

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

    std::vector<bool> used(candidates.size(), false);
    int plannedGain = 0;

    while (player.getBalance() + plannedGain < targetAmount) {
        const int shortage = targetAmount - (player.getBalance() + plannedGain);
        showMessage(
            "Likuidasi aset dibutuhkan. Kekurangan dana: M" + std::to_string(shortage) + ".");

        std::vector<int> visibleIndices;
        for (int i = 0; i < static_cast<int>(candidates.size()); ++i) {
            if (used[i]) {
                continue;
            }

            const LiquidationCandidate& candidate = candidates[i];
            if (candidate.sellValue > 0) {
                visibleIndices.push_back(i);
                showMessage(
                    std::to_string(static_cast<int>(visibleIndices.size())) +
                    ". Jual ke Bank " + candidate.name +
                    " (" + candidate.code + ") - M" + std::to_string(candidate.sellValue));
            }
            if (candidate.mortgageValue > 0) {
                visibleIndices.push_back(i);
                showMessage(
                    std::to_string(static_cast<int>(visibleIndices.size())) +
                    ". Gadai " + candidate.name +
                    " (" + candidate.code + ") - M" + std::to_string(candidate.mortgageValue));
            }
        }

        if (visibleIndices.empty()) {
            decisions.clear();
            return false;
        }

        showMessage("0. Batal dan kosongkan seluruh rencana likuidasi");
        const int choice = promptIntInRange(
            "Pilih opsi likuidasi: ",
            0,
            static_cast<int>(visibleIndices.size()));

        if (choice == 0) {
            decisions.clear();
            return false;
        }

        int currentDisplay = 0;
        for (int i = 0; i < static_cast<int>(candidates.size()); ++i) {
            if (used[i]) {
                continue;
            }

            const LiquidationCandidate& candidate = candidates[i];
            if (candidate.sellValue > 0) {
                ++currentDisplay;
                if (currentDisplay == choice) {
                    used[i] = true;
                    plannedGain += candidate.sellValue;
                    decisions.push_back({candidate.tileIndex, LiquidationActionKind::Sell});
                    break;
                }
            }
            if (candidate.mortgageValue > 0) {
                ++currentDisplay;
                if (currentDisplay == choice) {
                    used[i] = true;
                    plannedGain += candidate.mortgageValue;
                    decisions.push_back({candidate.tileIndex, LiquidationActionKind::Mortgage});
                    break;
                }
            }
        }
    }

    return true;
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
    for (int index : validTileIndices) {
        showMessage(std::to_string(index + 1) + ". Tile " + std::to_string(index + 1));
    }

    if (allowCancel) {
        showMessage("0. Batal");
        const int choice = promptIntInRange(
            "Pilih tile (0-40): ",
            0,
            40);
        return choice - 1;
    }

    const int choice = promptIntInRange(
        "Pilih tile (1-40): ",
        1,
        40);

    return choice - 1;
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
