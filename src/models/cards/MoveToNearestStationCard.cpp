#include "models/cards/MoveToNearestStationCard.hpp"

#include "core/Board.hpp"
#include "core/GameContext.hpp"
#include "models/Player.hpp"
#include "models/tiles/GoTile.hpp"
#include "models/tiles/RailroadTile.hpp"

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
        GoTile* goTile = dynamic_cast<GoTile*>(board->getTile("GO"));
        if (goTile != nullptr) {
            goTile->awardSalary(player);
        }
    }

    nearestRailroad->onLanded(player, gameContext);
}
