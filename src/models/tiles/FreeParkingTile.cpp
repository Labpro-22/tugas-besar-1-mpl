#include "models/tiles/FreeParkingTile.hpp"

#include "core/GameContext.hpp"

FreeParkingTile::FreeParkingTile() : ActionTile() {}

FreeParkingTile::FreeParkingTile(int index, const std::string& code, const std::string& name)
    : ActionTile(index, code, name) {}

void FreeParkingTile::onLanded(Player& player, GameContext& gameContext) {
    gameContext.showMessage("Kamu berhenti di Bebas Parkir. Tidak ada aksi tambahan.");
    gameContext.logEvent("PARKIR", player.getUsername() + " berhenti di Parkir Gratis.");
}
