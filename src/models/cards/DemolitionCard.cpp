#include "models/cards/DemolitionCard.hpp"

#include <vector>

#include "core/GameContext.hpp"
#include "core/GameIO.hpp"
#include "core/TurnManager.hpp"
#include "models/Player.hpp"
#include "models/tiles/PropertyTile.hpp"
#include "models/tiles/StreetTile.hpp"
#include "utils/exceptions/NimonspoliException.hpp"

namespace {
    std::string buildingLabel(int level) {
        if (level == 5) {
            return "hotel";
        }
        return std::to_string(level) + " rumah";
    }
}

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

    std::vector<StreetTile*> targetProperties;
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
            StreetTile* street = property == nullptr ? nullptr : property->asStreetTile();
            if (street == nullptr || street->getBuildingLevel() <= 0) {
                continue;
            }

            targetProperties.push_back(street);
            targetOwners.push_back(otherPlayer);
        }
    }

    if (targetProperties.empty()) {
        throw SkillUseFailedException(
            getTypeName(),
            "tidak ada bangunan milik lawan yang dapat dihancurkan.");
    }

    int choice = -1;
    if (io->usesRichGuiPresentation()) {
        std::vector<int> validTileIndices;
        validTileIndices.reserve(targetProperties.size());
        for (StreetTile* property : targetProperties) {
            if (property != nullptr) {
                validTileIndices.push_back(property->getIndex());
            }
        }

        const int selectedTileIndex = io->promptTileSelection(
            "Pilih properti lawan yang bangunannya ingin dihancurkan langsung dari board.",
            validTileIndices);

        for (int i = 0; i < static_cast<int>(targetProperties.size()); ++i) {
            if (targetProperties[i] != nullptr && targetProperties[i]->getIndex() == selectedTileIndex) {
                choice = i;
                break;
            }
        }
    } else {
        gameContext.showMessage("Pilih properti lawan yang bangunannya ingin dihancurkan:");
        for (int i = 0; i < static_cast<int>(targetProperties.size()); ++i) {
            gameContext.showMessage(
                std::to_string(i + 1) + ". "
                    + targetOwners[i]->getUsername()
                    + " - " + targetProperties[i]->getName()
                    + " (" + targetProperties[i]->getCode() + ")"
                    + " - " + buildingLabel(targetProperties[i]->getBuildingLevel()));
        }

        choice = gameContext.promptIntInRange(
            "Pilihan (1-" + std::to_string(targetProperties.size()) + "): ",
            1,
            static_cast<int>(targetProperties.size())) - 1;
    }

    if (choice < 0) {
        return;
    }

    StreetTile* targetProperty = targetProperties[choice];
    Player* owner = targetOwners[choice];
    int destroyedLevel = targetProperty->getBuildingLevel();
    targetProperty->setBuildingLevel(0);

    gameContext.showMessage(
        "BOOOMMMM!!! Semua bangunan di " + targetProperty->getName()
            + " (" + targetProperty->getCode() + ") milik "
            + owner->getUsername() + " sudah rata dengan tanah.");
    gameContext.logEvent(
        "KARTU",
        player.getUsername() + " menggunakan DemolitionCard untuk menghancurkan " +
            buildingLabel(destroyedLevel) +
            " di " + targetProperty->getCode() + " milik " + owner->getUsername() + ".");
}
