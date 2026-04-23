#include "models/cards/MoveBackCard.hpp"

#include <string>

#include "core/Board.hpp"
#include "core/GameContext.hpp"
#include "core/GameIO.hpp"
#include "core/MovementService.hpp"
#include "models/Player.hpp"
#include "models/tiles/Tile.hpp"

MoveBackCard::MoveBackCard()
    : MoveBackCard(3) {}

MoveBackCard::MoveBackCard(int steps)
    : ActionCard("Mundur " + std::to_string(steps) + " petak."),
      steps(steps) {}

int MoveBackCard::getSteps() const {
    return steps;
}

void MoveBackCard::execute(Player& player, GameContext& gameContext) {
    if (player.isShieldActive()) {
        gameContext.showMessage("[SHIELD ACTIVE]: Efek ShieldCard melindungi Anda dari kartu mundur.");
        gameContext.logEvent(
            "KARTU",
            player.getUsername() + " terlindungi ShieldCard dari MoveBackCard.");
        return;
    }

    Board* board = gameContext.getBoard();
    if (board == nullptr || board->getTileCount() <= 0) {
        return;
    }

    int tileCount = board->getTileCount();
    int targetIndex = (player.getPosition() - (steps % tileCount) + tileCount) % tileCount;
    player.moveTo(targetIndex);

    Tile* targetTile = board->getTile(targetIndex);
    if (targetTile != nullptr) {
        if (gameContext.getIO() != nullptr) {
            gameContext.getIO()->showPawnStep(player, targetIndex);
        }
        gameContext.showMessage(
            "Bidak dipindahkan ke " + targetTile->getName() +
                " (" + targetTile->getCode() + ").");
        if (MovementService::shouldSkipGoLandingSalary(targetTile)) {
            gameContext.showMessage("Pergerakan mundur ke GO tidak memberikan gaji.");
            return;
        }
        targetTile->onLanded(player, gameContext);
    }
}
