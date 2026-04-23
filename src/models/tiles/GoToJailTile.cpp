#include "models/tiles/GoToJailTile.hpp"
#include "models/Player.hpp"
#include "core/Board.hpp"
#include "core/GameContext.hpp"
#include "core/GameIO.hpp"
#include "models/tiles/Tile.hpp"
#include "utils/TransactionLogger.hpp"
#include "core/TurnManager.hpp"

GoToJailTile::GoToJailTile() : ActionTile() {}

GoToJailTile::GoToJailTile(int index, const std::string& code, const std::string& name)
    : ActionTile(index, code, name, TileCategory::DEFAULT) {}

void GoToJailTile::onLanded(Player& player, GameContext& gameContext) {
    if (gameContext.getIO() != nullptr) {
        gameContext.getIO()->showMessage("Kamu mendarat di Petak Pergi ke Penjara!");
    }

    if (player.isShieldActive()) {
        if (gameContext.getIO() != nullptr) {
            gameContext.getIO()->showMessage("[SHIELD ACTIVE]: Efek ShieldCard melindungi Anda dari sanksi penjara.");
        }
        gameContext.logEvent(
            "PENJARA",
            player.getUsername() + " terlindungi ShieldCard dari petak PPJ.");
        return;
    }

    if (gameContext.getIO() != nullptr) {
        gameContext.getIO()->showMessage("Bidak dipindahkan ke Penjara.");
    }

    Board* board = gameContext.getBoard();
    Tile* jailTile = board == nullptr ? nullptr : board->getTile("PEN");
    if (jailTile != nullptr) {
        player.setPosition(jailTile->getIndex());
    } else {
        player.setPosition(10);
    }
    player.setStatus(PlayerStatus::JAILED);
    player.setJailTurns(0);
    
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

std::string GoToJailTile::getDisplayLabel() const {
    return getCode(); // "PPJ"
}
