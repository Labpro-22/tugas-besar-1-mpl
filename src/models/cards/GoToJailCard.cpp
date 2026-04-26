#include "models/cards/GoToJailCard.hpp"

#include "core/Board.hpp"
#include "core/GameContext.hpp"
#include "core/GameIO.hpp"
#include "models/Player.hpp"
#include "models/tiles/Tile.hpp"

GoToJailCard::GoToJailCard()
    : ActionCard("Awkoakwoak Anda masuk Penjara.") {}

void GoToJailCard::execute(Player& player, GameContext& gameContext) {
    if (player.consumeShield()) {
        gameContext.showMessage("[SHIELD ACTIVE]: Alhamdulillah, ShieldCard melindungimu dari masuk Penjara!");
        gameContext.logEvent(
            "KARTU",
            player.getUsername() + " terlindungi ShieldCard dari GoToJailCard.");
        return;
    }

    gameContext.showMessage("Awokwokwowk anda masuk penjara.");
    gameContext.showMessage("Bidak dipindahkan ke Penjara.");

    Board* board = gameContext.getBoard();
    Tile* jailTileBase = board == nullptr ? nullptr : board->getTile("PEN");
    if (jailTileBase != nullptr) {
        jailTileBase->applyJailStatus(player);
        if (gameContext.getIO() != nullptr) {
            gameContext.getIO()->showPawnStep(player, jailTileBase->getIndex());
        }
        gameContext.logEvent(
            "KARTU",
            player.getUsername() + " terkena GoToJailCard dan masuk Penjara.");
        return;
    }

    player.setStatus(PlayerStatus::JAILED);
    player.setJailTurns(0);
    player.setConsecutiveDoubles(0);
    gameContext.logEvent(
        "KARTU",
        player.getUsername() + " terkena GoToJailCard dan masuk Penjara.");
}
