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
#include "utils/OutputFormatter.hpp"

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
        OutputFormatter::formatMoney(finalPrice);
    if (finalPrice != originalPrice) {
        prompt += " (setelah diskon dari " + OutputFormatter::formatMoney(originalPrice) + ")";
    }
    prompt += "?";

    return confirmYN(prompt);
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
