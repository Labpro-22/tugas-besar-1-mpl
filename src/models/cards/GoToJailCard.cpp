#include "models/cards/GoToJailCard.hpp"

#include "core/Board.hpp"
#include "core/GameContext.hpp"
#include "core/GameIO.hpp"
#include "models/Player.hpp"
#include "models/tiles/Tile.hpp"

GoToJailCard::GoToJailCard()
    : ActionCard("Awkoakwoak Anda masuk Penjara.") {}

void GoToJailCard::execute(Player& player, GameContext& gameContext) {
    if (player.isShieldActive()) {
        if (gameContext.getIO() != nullptr) {
            gameContext.getIO()->showMessage("[SHIELD ACTIVE]: Efek ShieldCard melindungi Anda dari kartu masuk penjara.");
        }
        gameContext.logEvent(
            "KARTU",
            player.getUsername() + " terlindungi ShieldCard dari GoToJailCard.");
        return;
    }

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
        player.moveTo(jailTileBase->getIndex());
        player.setStatus(PlayerStatus::JAILED);
        player.setJailTurns(0);
        player.setConsecutiveDoubles(0);
        return;
    }
    player.setStatus(PlayerStatus::JAILED);
    player.setJailTurns(0);
}
