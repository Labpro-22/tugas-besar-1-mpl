#include "models/cards/DemolitionCard.hpp"

#include <vector>

#include "core/GameContext.hpp"
#include "core/TurnManager.hpp"
#include "models/Player.hpp"
#include "models/tiles/PropertyTile.hpp"
#include "utils/exceptions/NimonspoliException.hpp"

DemolitionCard::DemolitionCard()
    : SkillCard() {}

std::string DemolitionCard::getTypeName() const {
    return "DemolitionCard";
}

void DemolitionCard::use(Player& player, GameContext& gameContext) {
    TurnManager* turnManager = gameContext.getTurnManager();
    if (turnManager == nullptr || !gameContext.hasIO()) {
        return;
    }

    std::vector<PropertyTile*> targetProperties;
    std::vector<Player*> targetOwners;
    std::vector<Player*> activePlayers = turnManager->getActivePlayers();

    for (Player* otherPlayer : activePlayers) {
        if (otherPlayer == nullptr || otherPlayer == &player || otherPlayer->isBankrupt()) {
            continue;
        }

        if (otherPlayer->isShieldActive()) {
            continue;
        }

        const std::vector<PropertyTile*>& properties = otherPlayer->getProperties();
        for (PropertyTile* property : properties) {
            if (property == nullptr) {
                continue;
            }

            targetProperties.push_back(property);
            targetOwners.push_back(otherPlayer);
        }
    }

    if (targetProperties.empty()) {
        throw SkillUseFailedException(
            getTypeName(),
            "tidak ada properti lawan yang dapat dihancurkan.");
    }

    gameContext.showMessage("Pilih properti lawan yang ingin dihancurkan:");
    for (int i = 0; i < static_cast<int>(targetProperties.size()); ++i) {
        gameContext.showMessage(
            std::to_string(i + 1) + ". "
                + targetOwners[i]->getUsername()
                + " - " + targetProperties[i]->getName()
                + " (" + targetProperties[i]->getCode() + ")");
    }

    int choice = gameContext.promptIntInRange(
        "Pilihan (1-" + std::to_string(targetProperties.size()) + "): ",
        1,
        static_cast<int>(targetProperties.size()));

    PropertyTile* targetProperty = targetProperties[choice - 1];
    Player* owner = targetOwners[choice - 1];

    targetProperty->setBuildingLevel(0);
    targetProperty->setFestivalState(1, 0);
    targetProperty->returnToBank();

    gameContext.showMessage(
        targetProperty->getName()
            + " (" + targetProperty->getCode() + ") milik "
            + owner->getUsername()
            + " telah dihancurkan dan dikembalikan ke Bank.");
}
