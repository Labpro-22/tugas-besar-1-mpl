#include "models/tiles/FreeParkingTile.hpp"

#include "core/GameContext.hpp"

FreeParkingTile::FreeParkingTile() : ActionTile() {}

FreeParkingTile::FreeParkingTile(int index, const std::string& code, const std::string& name)
    : ActionTile(index, code, name, TileCategory::DEFAULT) {}

void FreeParkingTile::onLanded(Player& player, GameContext& gameContext) {
    gameContext.logEvent("PARKIR", player.getUsername() + " berhenti di Parkir Gratis.");
}

std::string FreeParkingTile::getDisplayLabel() const {
    return getCode();
}
