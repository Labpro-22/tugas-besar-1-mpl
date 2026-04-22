#include "models/tiles/GoToJailTile.hpp"
#include "models/Player.hpp"
#include "core/GameContext.hpp"
#include "utils/TransactionLogger.hpp"
#include "core/TurnManager.hpp"

GoToJailTile::GoToJailTile() : ActionTile() {}

GoToJailTile::GoToJailTile(int index, const std::string& code, const std::string& name)
    : ActionTile(index, code, name, TileCategory::DEFAULT) {}

void GoToJailTile::onLanded(Player& player, GameContext& gameContext) {
    player.setPosition(10); 
    player.setStatus(PlayerStatus::JAILED);
    player.setJailTurns(0);
    
    int currentTurn = gameContext.getTurnManager()->getCurrentTurn();
    gameContext.getLogger()->log(currentTurn,player.getUsername(), "PENJARA", "Masuk penjara dari petak PPJ");
}

std::string GoToJailTile::getDisplayLabel() const {
    return getCode(); // "PPJ"
}
