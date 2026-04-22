#include "models/tiles/FreeParkingTile.hpp"

#include "core/GameContext.hpp"
#include "core/GameIO.hpp"

FreeParkingTile::FreeParkingTile() : ActionTile() {}

FreeParkingTile::FreeParkingTile(int index, const std::string& code, const std::string& name)
    : ActionTile(index, code, name, TileCategory::DEFAULT) {}

void FreeParkingTile::onLanded(Player& player, GameContext& gameContext) {
    if (gameContext.getIO() != nullptr) {
        gameContext.getIO()->showMessage("Kamu berhenti di Bebas Parkir. Tidak ada aksi tambahan.");
    }
    gameContext.logEvent("PARKIR", player.getUsername() + " berhenti di Parkir Gratis.");
}

std::string FreeParkingTile::getDisplayLabel() const {
    return getCode();
}
