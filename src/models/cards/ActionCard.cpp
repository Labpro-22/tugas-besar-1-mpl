#include "models/cards/ActionCard.hpp"

ActionCard::ActionCard() : text("") {}

ActionCard::ActionCard(const std::string& text)
    : text(text) {}

const std::string& ActionCard::getText() const {
    return text;
}
