#include "models/cards/LassoCard.hpp"

#include <vector>

#include "core/Board.hpp"
#include "core/GameContext.hpp"
#include "core/GameIO.hpp"
#include "core/MovementService.hpp"
#include "core/TurnManager.hpp"
#include "models/Player.hpp"
#include "models/tiles/Tile.hpp"
#include "utils/exceptions/NimonspoliException.hpp"

LassoCard::LassoCard()
    : SkillCard() {}

std::string LassoCard::getTypeName() const {
    return "LassoCard";
}

void LassoCard::use(Player& player, GameContext& gameContext) {
    Board* board = gameContext.getBoard();
    TurnManager* turnManager = gameContext.getTurnManager();
    GameIO* io = gameContext.getIO();

    if (board == nullptr || turnManager == nullptr || board->getTileCount() <= 0) {
        return;
    }

    Player* target = nullptr;
    int boardSize = board->getTileCount();
    int nearestDistance = boardSize + 1;
    std::vector<Player*> activePlayers = turnManager->getActivePlayers();

    for (Player* otherPlayer : activePlayers) {
        if (otherPlayer == nullptr || otherPlayer == &player || !otherPlayer->isActive()) {
            continue;
        }

        if (otherPlayer->isShieldActive()) {
            continue;
        }

        if (otherPlayer->isJailed()) {
            continue;
        }

        int distance = (otherPlayer->getPosition() - player.getPosition() + boardSize) % boardSize;

        if (distance > 0 && distance < nearestDistance) {
            target = otherPlayer;
            nearestDistance = distance;
        }
    }

    if (target == nullptr) {
        throw SkillUseFailedException(
            getTypeName(),
            "tidak ada pemain lawan di depan posisi kamu yang dapat ditarik.");
    }

    int destinationIndex = player.getPosition();
    Tile* destinationTile = board->getTile(destinationIndex);

    target->moveTo(destinationIndex);

    if (io != nullptr) {
        io->showMessage(target->getUsername() + " ditarik ke posisi " +
                        player.getUsername() + ".");
    }

    if (destinationTile != nullptr) {
        if (MovementService::shouldSkipGoLandingSalary(destinationTile)) {
            if (io != nullptr) {
                io->showMessage("Tarikan ke GO tidak memberikan gaji.");
            }
            return;
        }
        destinationTile->onLanded(*target, gameContext);
    }
}
