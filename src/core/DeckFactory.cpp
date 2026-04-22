#include "core/DeckFactory.hpp"

#include <random>
#include <sstream>
#include <string>
#include <vector>

#include "models/cards/ActionCard.hpp"
#include "models/cards/BirthdayCard.hpp"
#include "models/cards/CampaignCard.hpp"
#include "models/cards/DemolitionCard.hpp"
#include "models/cards/DiscountCard.hpp"
#include "models/cards/DoctorFeeCard.hpp"
#include "models/cards/GoToJailCard.hpp"
#include "models/cards/LassoCard.hpp"
#include "models/cards/MoveBackCard.hpp"
#include "models/cards/MoveCard.hpp"
#include "models/cards/MoveToNearestStationCard.hpp"
#include "models/cards/ShieldCard.hpp"
#include "models/cards/SkillCard.hpp"
#include "models/cards/TeleportCard.hpp"

namespace {
    int randomInt(int minValue, int maxValue) {
        static std::mt19937 generator(std::random_device{}());
        std::uniform_int_distribution<int> distribution(minValue, maxValue);
        return distribution(generator);
    }

    std::string getCardType(const std::string& encodedName) {
        std::size_t separator = encodedName.find(':');
        if (separator == std::string::npos) {
            return encodedName;
        }
        return encodedName.substr(0, separator);
    }

    int getEncodedValue(const std::string& encodedName, int fallback) {
        std::size_t separator = encodedName.find(':');
        if (separator == std::string::npos || separator + 1 >= encodedName.size()) {
            return fallback;
        }

        try {
            std::size_t parsed = 0;
            int value = std::stoi(encodedName.substr(separator + 1), &parsed);
            return parsed == encodedName.size() - separator - 1 ? value : fallback;
        } catch (const std::exception&) {
            return fallback;
        }
    }
}

void DeckFactory::buildActionDecks(
    CardDeck<ActionCard>& chanceDeck,
    CardDeck<ActionCard>& communityDeck
) {
    std::vector<ActionCard*> chanceCards;
    chanceCards.push_back(new MoveToNearestStationCard());
    chanceCards.push_back(new MoveBackCard());
    chanceCards.push_back(new GoToJailCard());

    std::vector<ActionCard*> communityCards;
    communityCards.push_back(new BirthdayCard(100));
    communityCards.push_back(new DoctorFeeCard(700));
    communityCards.push_back(new CampaignCard(200));

    chanceDeck.initializeDeck(chanceCards);
    communityDeck.initializeDeck(communityCards);
    chanceDeck.reshuffle();
    communityDeck.reshuffle();
}

void DeckFactory::buildSkillDeck(CardDeck<SkillCard>& skillDeck) {
    std::vector<SkillCard*> skillCards;
    for (int i = 0; i < 4; ++i) {
        skillCards.push_back(new MoveCard(randomInt(1, 12), 0));
    }
    for (int i = 0; i < 3; ++i) {
        skillCards.push_back(new DiscountCard(randomInt(1, 100), 1));
    }
    for (int i = 0; i < 2; ++i) {
        skillCards.push_back(new ShieldCard());
        skillCards.push_back(new TeleportCard());
        skillCards.push_back(new LassoCard());
        skillCards.push_back(new DemolitionCard());
    }

    skillDeck.initializeDeck(skillCards);
    skillDeck.reshuffle();
}

void DeckFactory::buildSkillDeckFromState(
    CardDeck<SkillCard>& skillDeck,
    const std::vector<std::string>& deckState
) {
    if (deckState.empty()) {
        buildSkillDeck(skillDeck);
        return;
    }

    std::vector<SkillCard*> skillCards;
    for (const std::string& cardName : deckState) {
        SkillCard* card = createSkillCardByName(cardName);
        if (card != nullptr) {
            skillCards.push_back(card);
        }
    }
    skillDeck.initializeDeck(skillCards);
}

SkillCard* DeckFactory::createSkillCardByName(const std::string& typeName) {
    std::string type = getCardType(typeName);
    if (type == "MoveCard") {
        return new MoveCard(getEncodedValue(typeName, randomInt(1, 12)), 0);
    }
    if (type == "DiscountCard") {
        return new DiscountCard(getEncodedValue(typeName, randomInt(10, 50)), 1);
    }
    if (type == "ShieldCard") return new ShieldCard();
    if (type == "TeleportCard") return new TeleportCard();
    if (type == "LassoCard") return new LassoCard();
    if (type == "DemolitionCard") return new DemolitionCard();
    return nullptr;
}

std::string DeckFactory::encodeSkillCard(const SkillCard* card) {
    if (card == nullptr) {
        return "";
    }

    const std::string typeName = card->getTypeName();
    if (typeName == "MoveCard" || typeName == "DiscountCard") {
        return typeName + ":" + std::to_string(card->getValue());
    }
    return typeName;
}

std::string DeckFactory::describeSkillCard(const SkillCard* card) {
    if (card == nullptr) return "";

    const std::string typeName = card->getTypeName();
    if (typeName == "MoveCard") return "Maju " + std::to_string(card->getValue()) + " petak";
    if (typeName == "DiscountCard") return "Diskon pembayaran " + std::to_string(card->getValue()) + "%";
    if (typeName == "ShieldCard") return "Kebal tagihan atau sanksi selama 1 turn";
    if (typeName == "TeleportCard") return "Pindah ke petak manapun";
    if (typeName == "LassoCard") return "Menarik lawan terdekat di depan";
    if (typeName == "DemolitionCard") return "Menghancurkan properti lawan";
    return "Kartu kemampuan";
}
