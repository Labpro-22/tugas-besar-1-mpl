#include "core/MovementService.hpp"

#include "core/Board.hpp"
#include "core/GameContext.hpp"
#include "models/Player.hpp"
#include "models/tiles/Tile.hpp"

void MovementService::awardGoSalaryForForwardMovement(
    Board& board,
    Player& player,
    GameContext& gameContext,
    int oldPosition,
    int targetIndex
) {
    if (targetIndex == 0 || targetIndex >= oldPosition) {
        return;
    }

    Tile* goTile = board.getTile("GO");
    if (goTile != nullptr) {
        goTile->onPassed(player, gameContext);
    }
}

bool MovementService::shouldSkipGoLandingSalary(const Tile* tile) {
    return tile != nullptr && tile->getCode() == "GO";
}
