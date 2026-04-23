#include "models/cards/GoToJailCard.hpp"

#include "core/Board.hpp"
#include "core/GameContext.hpp"
#include "models/Player.hpp"
#include "models/tiles/Tile.hpp"

GoToJailCard::GoToJailCard()
    : ActionCard("Awkoakwoak Anda masuk Penjara.") {}

void GoToJailCard::execute(Player& player, GameContext& gameContext) {
    if (player.isShieldActive()) {
        gameContext.showMessage("[SHIELD ACTIVE]: Efek ShieldCard melindungi Anda dari kartu masuk penjara.");
        gameContext.logEvent(
            "KARTU",
            player.getUsername() + " terlindungi ShieldCard dari GoToJailCard.");
        return;
    }

    gameContext.showMessage("Bidak dipindahkan ke Penjara.");

    Board* board = gameContext.getBoard();
    Tile* jailTileBase = board == nullptr ? nullptr : board->getTile("PEN");
    if (jailTileBase != nullptr) {
        jailTileBase->applyJailStatus(player);
        return;
    }

    player.setPosition(10);
    player.setStatus(PlayerStatus::JAILED);
    player.setJailTurns(0);
    player.setConsecutiveDoubles(0);
}
