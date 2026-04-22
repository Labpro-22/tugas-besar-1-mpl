#pragma once

#include <string>

#include "utils/CardDeck.hpp"

class ActionCard;
class SkillCard;

class DeckFactory {
public:
    static void buildActionDecks(
        CardDeck<ActionCard>& chanceDeck,
        CardDeck<ActionCard>& communityDeck
    );

    static void buildSkillDeck(CardDeck<SkillCard>& skillDeck);
    static SkillCard* createSkillCardByName(const std::string& typeName);
    static std::string describeSkillCard(const SkillCard* card);
};
