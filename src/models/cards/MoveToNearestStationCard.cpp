#include "models/cards/MoveToNearestStationCard.hpp"

#include "core/Board.hpp"
#include "core/GameContext.hpp"
#include "core/MovementService.hpp"
#include "models/Player.hpp"
#include "models/tiles/RailroadTile.hpp"
#include "models/tiles/Tile.hpp"

MoveToNearestStationCard::MoveToNearestStationCard()
    : ActionCard("Pergi ke stasiun terdekat.") {}

void MoveToNearestStationCard::execute(Player& player, GameContext& gameContext) {
    Board* board = gameContext.getBoard();
    if (board == nullptr) {
        return;
    }

    RailroadTile* nearestRailroad = board->getNearestRailroad(player.getPosition());
    if (nearestRailroad == nullptr) {
        return;
    }

    int oldPosition = player.getPosition();
    int targetIndex = nearestRailroad->getIndex();
    bool passedGo = targetIndex <= oldPosition;
    player.moveTo(targetIndex);
    if (passedGo) {
        MovementService::awardGoSalaryForForwardMovement(
            *board,
            player,
            gameContext,
            oldPosition,
            targetIndex
        );
    }

    gameContext.showMessage(
        "Bidak dipindahkan ke stasiun terdekat: " + nearestRailroad->getName() +
            " (" + nearestRailroad->getCode() + ").");
    nearestRailroad->onLanded(player, gameContext);
}
