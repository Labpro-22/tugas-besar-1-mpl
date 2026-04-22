#include "models/cards/DemolitionCard.hpp"

#include <iostream>
#include <limits>
#include <vector>

#include "core/GameContext.hpp"
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
    if (turnManager == nullptr) {
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
        std::cout << "Tidak ada properti lawan yang dapat dihancurkan.\n";
        return;
    }

    std::cout << "Pilih properti lawan yang ingin dihancurkan:\n";
    for (int i = 0; i < static_cast<int>(targetProperties.size()); ++i) {
        std::cout << i + 1 << ". "
                  << targetOwners[i]->getUsername()
                  << " - " << targetProperties[i]->getName()
                  << " (" << targetProperties[i]->getCode() << ")\n";
    }

    int choice = 0;
    while (true) {
        std::cout << "Pilihan (1-" << targetProperties.size() << "): ";
        if (std::cin >> choice && choice >= 1 && choice <= static_cast<int>(targetProperties.size())) {
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            break;
        }

        std::cout << "Pilihan tidak valid.\n";
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }

    PropertyTile* targetProperty = targetProperties[choice - 1];
    Player* owner = targetOwners[choice - 1];

    owner->removeProperty(targetProperty);
    targetProperty->returnToBank();
    player.setUsedSkillThisTurn(true);

    std::cout << targetProperty->getName()
              << " (" << targetProperty->getCode() << ") milik "
              << owner->getUsername()
              << " telah dihancurkan dan dikembalikan ke Bank.\n";
}
