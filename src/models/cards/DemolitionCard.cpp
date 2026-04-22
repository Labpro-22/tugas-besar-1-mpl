#include "models/cards/DemolitionCard.hpp"

#include <vector>

#include "core/GameContext.hpp"
#include "core/GameIO.hpp"
#include "core/TurnManager.hpp"
#include "models/Player.hpp"
#include "models/tiles/PropertyTile.hpp"

DemolitionCard::DemolitionCard()
    : SkillCard() {}

std::string DemolitionCard::getTypeName() const {
    return "DemolitionCard";
}

void DemolitionCard::use(Player& player, GameContext& gameContext) {
    TurnManager* turnManager = gameContext.getTurnManager();
    GameIO* io = gameContext.getIO();
    if (turnManager == nullptr || io == nullptr) {
        return;
    }

    std::vector<PropertyTile*> targetProperties;
    std::vector<Player*> targetOwners;
    std::vector<Player*> activePlayers = turnManager->getActivePlayers();

    for (Player* otherPlayer : activePlayers) {
        if (otherPlayer == nullptr || otherPlayer == &player || !otherPlayer->isActive()) {
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
        io->showMessage("Tidak ada properti lawan yang dapat dihancurkan.");
        return;
    }

    io->showMessage("Pilih properti lawan yang ingin dihancurkan:");
    for (int i = 0; i < static_cast<int>(targetProperties.size()); ++i) {
        io->showMessage(
            std::to_string(i + 1) + ". "
                + targetOwners[i]->getUsername()
                + " - " + targetProperties[i]->getName()
                + " (" + targetProperties[i]->getCode() + ")");
    }

    int choice = io->promptIntInRange(
        "Pilihan (1-" + std::to_string(targetProperties.size()) + "): ",
        1,
        static_cast<int>(targetProperties.size()));

    PropertyTile* targetProperty = targetProperties[choice - 1];
    Player* owner = targetOwners[choice - 1];

    owner->removeProperty(targetProperty);
    targetProperty->returnToBank();
    player.setUsedSkillThisTurn(true);

    io->showMessage(
        targetProperty->getName()
            + " (" + targetProperty->getCode() + ") milik "
            + owner->getUsername()
            + " telah dihancurkan dan dikembalikan ke Bank.");
}
