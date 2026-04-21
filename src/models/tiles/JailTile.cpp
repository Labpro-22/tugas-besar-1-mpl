#include "models/tiles/JailTile.hpp"
#include "models/Player.hpp"
#include "core/GameContext.hpp"

JailTile::JailTile() : ActionTile(), jailFine(0) {}

JailTile::JailTile(int index, const std::string& code, const std::string& name, int jailFine)
    : ActionTile(index, code, name, TileCategory::DEFAULT), jailFine(jailFine) {}

void JailTile::onLanded(Player& player, GameContext& gameContext) {
    if (player.getStatus() == PlayerStatus::ACTIVE) {
        // Hanya mampir
    } else if (player.getStatus() == PlayerStatus::JAILED) {
        processJailTurn(player, gameContext);
    }
}

void JailTile::processJailTurn(Player& player, GameContext& gameContext) const {
    // Logika UI untuk memilih bayar denda atau double dadu 
    // akan dipanggil oleh GameEngine, di sini kita hanya menyediakan utilitasnya.
}

std::string JailTile::getDisplayLabel() const {
    return getCode(); // "PEN"
}

int JailTile::getJailFine() const {
    return jailFine;
}