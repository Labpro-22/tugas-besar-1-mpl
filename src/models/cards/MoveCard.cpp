#include "models/cards/MoveCard.hpp"

#include "core/Board.hpp"
#include "core/GameContext.hpp"
#include "core/GameIO.hpp"
#include "models/Player.hpp"
#include "models/tiles/Tile.hpp"

MoveCard::MoveCard()
    : SkillCard() {}

MoveCard::MoveCard(int value, int remainingDuration)
    : SkillCard(value, remainingDuration) {}

std::string MoveCard::getTypeName() const {
    return "MoveCard";
}

bool MoveCard::canUseWhileJailed() const {
    return false;
}

void MoveCard::use(Player& player, GameContext& gameContext) {
    Board* board = gameContext.getBoard();
    if (board == nullptr || board->getTileCount() <= 0) {
        return;
    }

    int tileCount = board->getTileCount();
    int oldPosition = player.getPosition();
    int targetIndex = (player.getPosition() + getValue()) % tileCount;
    bool passedGo = oldPosition + getValue() >= tileCount;
    player.moveTo(targetIndex);
    player.setUsedSkillThisTurn(true);

    if (gameContext.getIO() != nullptr) {
        gameContext.getIO()->showMessage(
            "MoveCard digunakan! " + player.getUsername() +
                " maju " + std::to_string(getValue()) + " petak.");
    }

    if (passedGo && targetIndex != 0) {
        Tile* goTile = board->getTile("GO");
        if (goTile != nullptr) {
            goTile->onPassed(player, gameContext);
        }
    }

    Tile* targetTile = board->getTile(targetIndex);
    if (targetTile != nullptr) {
        if (gameContext.getIO() != nullptr) {
            gameContext.getIO()->showMessage(
                "Bidak mendarat di: " + targetTile->getName() +
                    " (" + targetTile->getCode() + ").");
        }
        targetTile->onLanded(player, gameContext);
    }
}
