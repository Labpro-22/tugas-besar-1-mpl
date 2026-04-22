#include "models/tiles/RailroadTile.hpp"
#include "core/GameContext.hpp"
#include "models/Player.hpp"

RailroadTile::RailroadTile() : PropertyTile() {}

RailroadTile::RailroadTile(
    int index, const std::string& code, const std::string& name, 
    int mortgageValue, const std::map<int, int>& rentTable
) : PropertyTile(index, code, name, 0, mortgageValue), rentTable(rentTable) {}

void RailroadTile::onLanded(Player& player, GameContext& gameContext) {
    if (getStatus() == PropertyStatus::BANK) {
        transferTo(player);
        gameContext.logEvent("RAILROAD", player.getUsername() + " mendapatkan " + getName() + " secara otomatis.");
    } else {
        gameContext.triggerRentEvent(player, *this);
    }
}

int RailroadTile::calculateRent(int diceTotal, const GameContext& gameContext) {
    if (getStatus() != PropertyStatus::OWNED) {
        return 0;
    }
    int count = gameContext.getRailroadCount(*getOwner());
    if (rentTable.find(count) != rentTable.end()) {
        return rentTable.at(count);
    }
    return 0;
}

int RailroadTile::getSellValueToBank() const {
    return 0; 
}

std::string RailroadTile::getDisplayLabel() const {
    return "[RAILROAD] " + getName();
}