#include "models/cards/SkillCard.hpp"

SkillCard::SkillCard()
    : value(0),
      remainingDuration(0) {}

SkillCard::SkillCard(int value, int remainingDuration)
    : value(value),
      remainingDuration(remainingDuration < 0 ? 0 : remainingDuration) {}

int SkillCard::getValue() const {
    return value;
}

int SkillCard::getRemainingDuration() const {
    return remainingDuration;
}

void SkillCard::setRemainingDuration(int remainingDuration) {
    this->remainingDuration = (remainingDuration < 0 ? 0 : remainingDuration);
}
