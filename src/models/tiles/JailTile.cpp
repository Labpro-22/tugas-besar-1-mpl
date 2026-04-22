#include "models/tiles/JailTile.hpp"
#include "models/Player.hpp"
#include "core/GameContext.hpp"

JailTile::JailTile() : ActionTile(), jailFine(0) {}

JailTile::JailTile(int index, const std::string& code, const std::string& name, int jailFine)
    : ActionTile(index, code, name, TileCategory::DEFAULT), jailFine(jailFine) {}

void JailTile::onLanded(Player& player, GameContext& gameContext) {
    if (player.getStatus() == PlayerStatus::ACTIVE) {
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
    gameContext.logEvent(
        "PENJARA",
        player.getUsername() + " sedang berada di Penjara. Denda keluar: M" + std::to_string(jailFine)
    );
}

std::string JailTile::getDisplayLabel() const {
    return getCode();
}

int JailTile::getJailFine() const {
    return jailFine;
}
