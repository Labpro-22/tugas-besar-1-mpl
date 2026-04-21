#include "models/tiles/GoToJailTile.hpp"
#include "models/Player.hpp"
#include "core/GameContext.hpp"
#include "utils/TransactionLogger.hpp"
#include "core/TurnManager.hpp"

GoToJailTile::GoToJailTile() : ActionTile() {}

GoToJailTile::GoToJailTile(int index, const std::string& code, const std::string& name)
    : ActionTile(index, code, name, TileCategory::DEFAULT) {}

void GoToJailTile::onLanded(Player& player, GameContext& gameContext) {
    // Ditendang ke penjara (indeks 11 kalo ga berubah)
    player.setPosition(11); 
    player.setStatus(PlayerStatus::JAILED);
    
    int currentTurn = gameContext.getTurnManager()->getCurrentTurn();
    gameContext.getLogger()->log(currentTurn,player.getUsername(), "PENJARA", "Masuk penjara dari petak PPJ");
}

std::string GoToJailTile::getDisplayLabel() const {
    return getCode(); // "PPJ"
}
