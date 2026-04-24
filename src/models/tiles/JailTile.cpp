#include "models/tiles/JailTile.hpp"
#include "models/Player.hpp"
#include "core/GameContext.hpp"
#include "utils/TextFormatter.hpp"

JailTile::JailTile() : ActionTile(), jailFine(0) {}

JailTile::JailTile(int index, const std::string& code, const std::string& name, int jailFine)
    : ActionTile(index, code, name), jailFine(jailFine) {}

void JailTile::onLanded(Player& player, GameContext& gameContext) {
    if (player.getStatus() == PlayerStatus::ACTIVE) {
        gameContext.showMessage("Kamu hanya mampir di Penjara.");
        gameContext.logEvent("PENJARA", player.getUsername() + " hanya mampir di Penjara.");
        return;
    }

    processJailTurn(player, gameContext);
}

void JailTile::applyJailStatus(Player& player) const {
    player.setPosition(getIndex());
    player.setStatus(PlayerStatus::JAILED);
    player.setJailTurns(0);
    player.setConsecutiveDoubles(0);
}

void JailTile::processJailTurn(Player& player, GameContext& gameContext) const {
    gameContext.showMessage(
        player.getUsername() + " sedang berada di Penjara. Denda keluar: " +
            TextFormatter::formatMoney(jailFine));
    gameContext.logEvent(
        "PENJARA",
        player.getUsername() + " sedang berada di Penjara. Denda keluar: " +
            TextFormatter::formatMoney(jailFine)
    );
}

int JailTile::getJailFine() const {
    return jailFine;
}
