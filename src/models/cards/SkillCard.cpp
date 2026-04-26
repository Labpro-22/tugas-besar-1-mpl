#include "models/cards/SkillCard.hpp"

SkillCard::SkillCard()
    : value(0) {}

SkillCard::SkillCard(int value)
    : value(value) {}

int SkillCard::getValue() const {
    return value;
}

bool SkillCard::canUseWhileJailed() const {
    return true;
}
