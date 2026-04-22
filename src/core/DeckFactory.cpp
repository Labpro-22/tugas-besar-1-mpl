#include "core/DeckFactory.hpp"

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
        skillCards.push_back(new MoveCard(4, 0));
    }
    for (int i = 0; i < 3; ++i) {
        skillCards.push_back(new DiscountCard(50, 1));
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

SkillCard* DeckFactory::createSkillCardByName(const std::string& typeName) {
    if (typeName == "MoveCard") return new MoveCard();
    if (typeName == "DiscountCard") return new DiscountCard(50, 1);
    if (typeName == "ShieldCard") return new ShieldCard();
    if (typeName == "TeleportCard") return new TeleportCard();
    if (typeName == "LassoCard") return new LassoCard();
    if (typeName == "DemolitionCard") return new DemolitionCard();
    return nullptr;
}

std::string DeckFactory::describeSkillCard(const SkillCard* card) {
    if (card == nullptr) return "";

    const std::string typeName = card->getTypeName();
    if (typeName == "MoveCard") return "Maju 4 petak";
    if (typeName == "DiscountCard") return "Diskon pembayaran 50%";
    if (typeName == "ShieldCard") return "Kebal tagihan atau sanksi selama 1 turn";
    if (typeName == "TeleportCard") return "Pindah ke petak manapun";
    if (typeName == "LassoCard") return "Menarik lawan terdekat di depan";
    if (typeName == "DemolitionCard") return "Menghancurkan properti lawan";
    return "Kartu kemampuan";
}
