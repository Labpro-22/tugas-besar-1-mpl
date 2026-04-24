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
    if (const JailTile* jailTile = dynamic_cast<const JailTile*>(jailTileBase)) {
        jailTile->applyJailStatus(player);
        if (gameContext.getIO() != nullptr) {
            gameContext.getIO()->showPawnStep(player, jailTile->getIndex());
        }
        return;
    }

    player.setPosition(10);
    player.setStatus(PlayerStatus::JAILED);
    player.setJailTurns(0);
    player.setConsecutiveDoubles(0);
    if (gameContext.getIO() != nullptr && jailTileBase != nullptr) {
        gameContext.getIO()->showPawnStep(player, jailTileBase->getIndex());
    }
}
