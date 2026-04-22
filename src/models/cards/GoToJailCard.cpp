#include "models/cards/GoToJailCard.hpp"

#include "core/Board.hpp"
#include "core/GameContext.hpp"
#include "core/GameIO.hpp"
#include "models/Player.hpp"
#include "models/tiles/JailTile.hpp"
#include "models/tiles/Tile.hpp"

GoToJailCard::GoToJailCard()
    : ActionCard("Awkoakwoak Anda masuk Penjara.") {}

void GoToJailCard::execute(Player& player, GameContext& gameContext) {
    if (gameContext.getIO() != nullptr) {
        gameContext.getIO()->showMessage("Bidak dipindahkan ke Penjara.");
    }

    Board* board = gameContext.getBoard();
    if (board == nullptr) {
        player.setStatus(PlayerStatus::JAILED);
        player.setJailTurns(0);
        return;
    }

    Tile* jailTileBase = board->getTile("PEN");

    if (jailTileBase != nullptr) {
        JailTile* jailTile = static_cast<JailTile*>(jailTileBase);
        player.moveTo(jailTile->getIndex());
        jailTile->applyJailStatus(player);
        return;
    }
    player.setStatus(PlayerStatus::JAILED);
    player.setJailTurns(0);
}
