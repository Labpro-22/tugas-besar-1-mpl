#include "models/cards/MoveCard.hpp"

#include "core/Board.hpp"
#include "core/GameContext.hpp"
#include "core/GameIO.hpp"
#include "core/MovementService.hpp"
#include "models/Player.hpp"
#include "models/tiles/Tile.hpp"

MoveCard::MoveCard()
    : SkillCard() {}

MoveCard::MoveCard(int value)
    : SkillCard(value) {}

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

    if (gameContext.getIO() != nullptr) {
        gameContext.getIO()->showPawnStep(player, targetIndex);
    }
    gameContext.showMessage(
        "MoveCard digunakan! " + player.getUsername() +
            " maju " + std::to_string(getValue()) + " petak.");
    gameContext.logEvent(
        "KARTU",
        player.getUsername() + " menggunakan MoveCard maju " +
            std::to_string(getValue()) + " petak.");

    if (passedGo) {
        MovementService::awardGoSalaryForForwardMovement(
            *board,
            player,
            gameContext,
            oldPosition,
            targetIndex
        );
    }

    Tile* targetTile = board->getTile(targetIndex);
    if (targetTile != nullptr) {
        gameContext.showMessage(
            "Bidak mendarat di: " + targetTile->getName() +
                " (" + targetTile->getCode() + ").");
        targetTile->onLanded(player, gameContext);
    }
}
