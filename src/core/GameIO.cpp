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
            "Pilih kartu (0-" + std::to_string(cards.size()) + "): ",
            0,
            static_cast<int>(cards.size()));
    }

    return promptIntInRange(
        "Pilih kartu (1-" + std::to_string(cards.size()) + "): ",
        1,
        static_cast<int>(cards.size()));
}
