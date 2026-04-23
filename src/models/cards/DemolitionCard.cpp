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

    std::vector<int> validTileIndices;
    validTileIndices.reserve(targetProperties.size());
    for (PropertyTile* property : targetProperties) {
        if (property != nullptr) {
            validTileIndices.push_back(property->getIndex());
        }
    }

    int selectedTileIndex = io->promptTileSelection(
        "Pilih properti lawan yang ingin dihancurkan langsung dari board.",
        validTileIndices);

    int choice = -1;
    for (int i = 0; i < static_cast<int>(targetProperties.size()); ++i) {
        if (targetProperties[i] != nullptr && targetProperties[i]->getIndex() == selectedTileIndex) {
            choice = i;
            break;
        }
    }

    if (choice < 0) {
        return;
    }

    PropertyTile* targetProperty = targetProperties[choice];
    Player* owner = targetOwners[choice];

    owner->removeProperty(targetProperty);
    targetProperty->returnToBank();
    player.setUsedSkillThisTurn(true);

    io->showMessage(
        targetProperty->getName()
            + " (" + targetProperty->getCode() + ") milik "
            + owner->getUsername()
            + " telah dihancurkan dan dikembalikan ke Bank.");
}
