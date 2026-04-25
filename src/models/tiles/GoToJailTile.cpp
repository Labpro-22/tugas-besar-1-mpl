#include "models/tiles/GoToJailTile.hpp"
#include "models/Player.hpp"
#include "core/Board.hpp"
#include "core/GameContext.hpp"
#include "models/tiles/Tile.hpp"
#include "utils/TransactionLogger.hpp"
#include "core/TurnManager.hpp"

GoToJailTile::GoToJailTile() : ActionTile() {}

GoToJailTile::GoToJailTile(int index, const std::string& code, const std::string& name)
    : ActionTile(index, code, name) {}

void GoToJailTile::onLanded(Player& player, GameContext& gameContext) {
    gameContext.showMessage("Kamu mendarat di Petak Pergi ke Penjara!");

    if (player.isShieldActive()) {
        gameContext.showMessage("[SHIELD ACTIVE]: Efek ShieldCard melindungi Anda dari sanksi penjara.");
        gameContext.logEvent(
            "PENJARA",
            player.getUsername() + " terlindungi ShieldCard dari petak PPJ.");
        return;
    }

    gameContext.showMessage("Bidak dipindahkan ke Penjara.");

    Board* board = gameContext.getBoard();
    Tile* jailTile = board == nullptr ? nullptr : board->getTile("PEN");
    if (jailTile != nullptr) {
        jailTile->applyJailStatus(player);
    } else {
        player.setPosition(10);
        player.setStatus(PlayerStatus::JAILED);
        player.setJailTurns(0);
        player.setConsecutiveDoubles(0);
    }
    
    if (gameContext.getLogger() != nullptr) {
        int currentTurn = 0;
        if (gameContext.getTurnManager() != nullptr) {
            currentTurn = gameContext.getTurnManager()->getCurrentTurn();
        }
        gameContext.getLogger()->log(
            currentTurn,
            player.getUsername(),
            "PENJARA",
            "Masuk penjara dari petak PPJ");
    }
}
