#include "core/GameIO.hpp"

#include <string>

#include "core/DeckFactory.hpp"
#include "models/cards/ActionCard.hpp"
#include "models/cards/SkillCard.hpp"
#include "models/Enums.hpp"
#include "models/Player.hpp"
#include "models/tiles/PropertyTile.hpp"

void GameIO::showPawnStep(const Player&, int)
{
}

void GameIO::showDiceRoll(int, int)
{
}

bool GameIO::confirmPropertyPurchase(const Player& player, const PropertyTile& property)
{
    const int originalPrice = property.getBuyPrice();
    const int finalPrice = player.getDiscountedAmount(originalPrice);

    if (!player.canAfford(finalPrice)) {
        return false;
    }

    std::string prompt = "Apakah kamu ingin membeli properti ini seharga M" +
        std::to_string(finalPrice);
    if (finalPrice != originalPrice) {
        prompt += " (setelah diskon dari M" + std::to_string(originalPrice) + ")";
    }
    prompt += "?";

    return confirmYN(prompt);
}

void GameIO::showPropertyNotice(const Player&, const PropertyTile& property)
{
    showMessage(property.getName());
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
            "Pilih kartu (0-" + std::to_string(cards.size()) + "): ",
            0,
            static_cast<int>(cards.size()));
    }

    return promptIntInRange(
        "Pilih kartu (1-" + std::to_string(cards.size()) + "): ",
        1,
        static_cast<int>(cards.size()));
}
