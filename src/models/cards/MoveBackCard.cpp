#include "models/cards/MoveBackCard.hpp"

#include "core/Board.hpp"
#include "core/GameContext.hpp"
#include "core/GameIO.hpp"
#include "models/Player.hpp"
#include "models/tiles/Tile.hpp"

MoveBackCard::MoveBackCard()
    : MoveBackCard(3) {}

MoveBackCard::MoveBackCard(int steps)
    : ActionCard("Anda mundur 3 petak."),
      steps(steps) {}

int MoveBackCard::getSteps() const {
    return steps;
}

void MoveBackCard::execute(Player& player, GameContext& gameContext) {
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
            gameContext.getIO()->showMessage(
                "Bidak dipindahkan ke " + targetTile->getName() +
                    " (" + targetTile->getCode() + ").");
        }
        targetTile->onLanded(player, gameContext);
    }
}
