#pragma once

#include <string>
#include <vector>

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
    static void buildSkillDeckFromState(
        CardDeck<SkillCard>& skillDeck,
        const std::vector<std::string>& deckState
    );
    static SkillCard* createSkillCardByName(const std::string& typeName);
    static std::string encodeSkillCard(const SkillCard* card);
    static std::string describeSkillCard(const SkillCard* card);
};
