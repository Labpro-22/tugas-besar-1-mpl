#include "models/cards/GoToJailCard.hpp"

#include "core/Board.hpp"
#include "core/GameContext.hpp"
#include "models/Player.hpp"
#include "models/tiles/JailTile.hpp"
#include "models/tiles/Tile.hpp"

GoToJailCard::GoToJailCard()
    : ActionCard("Awkoakwoak Anda masuk Penjara.") {}

void GoToJailCard::execute(Player& player, GameContext& gameContext) {
    Board* board = gameContext.getBoard();
    if (board == nullptr) {
        player.setStatus(PlayerStatus::JAILED);
        player.setJailTurns(0);
        return;
    }

    Tile* jailTileBase = board->getTile("PEN");
    JailTile* jailTile = dynamic_cast<JailTile*>(jailTileBase);

    if (jailTile != nullptr) {
        player.moveTo(jailTile->getIndex());
        jailTile->applyJailStatus(player);
        return;
    }

    if (jailTileBase != nullptr) {
        player.moveTo(jailTileBase->getIndex());
    }
    player.setStatus(PlayerStatus::JAILED);
    player.setJailTurns(0);
}
