#include "models/cards/LassoCard.hpp"

#include <iostream>
#include <vector>

#include "core/Board.hpp"
#include "core/GameContext.hpp"
#include "core/TurnManager.hpp"
#include "models/Player.hpp"
#include "models/tiles/Tile.hpp"

LassoCard::LassoCard()
    : SkillCard() {}

std::string LassoCard::getTypeName() const {
    return "LassoCard";
}

void LassoCard::use(Player& player, GameContext& gameContext) {
    Board* board = gameContext.getBoard();
    TurnManager* turnManager = gameContext.getTurnManager();
    if (board == nullptr || turnManager == nullptr || board->getTileCount() <= 0) {
        return;
    }

    Player* target = nullptr;
    int nearestDistance = board->getTileCount() + 1;
    std::vector<Player*> activePlayers = turnManager->getActivePlayers();

    for (Player* otherPlayer : activePlayers) {
        if (otherPlayer == nullptr || otherPlayer == &player || !otherPlayer->isActive()) {
            continue;
        }

        if (otherPlayer->isShieldActive()) {
            continue;
        }

        int distance = otherPlayer->getPosition() - player.getPosition();
        if (distance > 0 && distance < nearestDistance) {
            target = otherPlayer;
            nearestDistance = distance;
        }
    }

    if (target == nullptr) {
        std::cout << "Tidak ada pemain lawan di depan posisi Anda yang dapat ditarik.\n";
        return;
    }

    int destinationIndex = player.getPosition();
    Tile* destinationTile = board->getTile(destinationIndex);

    target->moveTo(destinationIndex);
    player.setUsedSkillThisTurn(true);

    std::cout << target->getUsername() << " ditarik ke posisi "
              << player.getUsername() << ".\n";

    if (destinationTile != nullptr) {
        destinationTile->onLanded(*target, gameContext);
    }
}
