#include "models/cards/TeleportCard.hpp"

#include <vector>

#include "core/Board.hpp"
#include "core/GameContext.hpp"
#include "core/GameIO.hpp"
#include "core/MovementService.hpp"
#include "models/Player.hpp"
#include "models/tiles/Tile.hpp"

TeleportCard::TeleportCard()
    : SkillCard() {}

std::string TeleportCard::getTypeName() const {
    return "TeleportCard";
}

bool TeleportCard::canUseWhileJailed() const {
    return false;
}

void TeleportCard::use(Player& player, GameContext& gameContext) {
    Board* board = gameContext.getBoard();
    if (board == nullptr || board->getTileCount() <= 0) {
        return;
    }

    int tileCount = board->getTileCount();
    GameIO* io = gameContext.getIO();
    if (io == nullptr) {
        return;
    }

    std::vector<int> validTileIndices;
    validTileIndices.reserve(tileCount);
    for (int index = 0; index < tileCount; ++index) {
        validTileIndices.push_back(index);
    }

    int targetIndex = io->promptTileSelection(
        "Pilih petak tujuan teleport langsung dari board.",
        validTileIndices);
    if (targetIndex < 0 || targetIndex >= tileCount) {
        return;
    }

    Tile* targetTile = board->getTile(targetIndex);
    if (targetTile == nullptr) {
        return;
    }

    int oldPosition = player.getPosition();
    player.moveTo(targetIndex);
    MovementService::awardGoSalaryForForwardMovement(
        *board,
        player,
        gameContext,
        oldPosition,
        targetIndex
    );

    io->showMessage(
        "Teleport berhasil! " + player.getUsername()
            + " berpindah ke " + targetTile->getName()
            + " (" + targetTile->getCode() + ").");

    targetTile->onLanded(player, gameContext);
}
