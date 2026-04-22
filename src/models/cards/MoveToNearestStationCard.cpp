#include "models/cards/MoveToNearestStationCard.hpp"

#include "core/Board.hpp"
#include "core/GameContext.hpp"
#include "core/GameIO.hpp"
#include "models/Player.hpp"
#include "models/tiles/GoTile.hpp"
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

    bool passedGo = player.moveTo(nearestRailroad->getIndex());
    if (passedGo) {
        Tile* goTile = board->getTile("GO");
        if (goTile != nullptr) {
            static_cast<GoTile*>(goTile)->awardSalary(player);
        }
    }

    if (gameContext.getIO() != nullptr) {
        gameContext.getIO()->showMessage(
            "Bidak dipindahkan ke stasiun terdekat: " + nearestRailroad->getName() +
                " (" + nearestRailroad->getCode() + ").");
    }
    nearestRailroad->onLanded(player, gameContext);
}
