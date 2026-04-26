#include "models/tiles/RailroadTile.hpp"
#include "core/GameContext.hpp"
#include "core/GameIO.hpp"
#include "models/Player.hpp"

RailroadTile::RailroadTile() : PropertyTile() {}

RailroadTile::RailroadTile(
    int index, const std::string& code, const std::string& name, 
    int mortgageValue, const std::map<int, int>& rentTable
) : PropertyTile(index, code, name, 0, mortgageValue), rentTable(rentTable) {}

void RailroadTile::onLanded(Player& player, GameContext& gameContext) {
    GameIO* io = gameContext.getIO();
    if (getStatus() == PropertyStatus::BANK) {
        if (io != nullptr) {
            io->showPropertyNotice(player, *this);
        }
        transferTo(player);
        gameContext.showMessage("Kamu mendarat di " + getName() + " (" + getCode() + ")!");
        gameContext.showMessage("Belum ada yang menginjaknya duluan, stasiun ini kini menjadi milikmu!");
        gameContext.logEvent(
            "BELI",
            getName() + " (" + getCode() + ") kini milik " + player.getUsername() + " (otomatis)");
    } else {
        gameContext.triggerRentEvent(player, *this);
    }
}

int RailroadTile::calculateRent(int, const GameContext& gameContext) {
    if (getStatus() != PropertyStatus::OWNED) {
        return 0;
    }
    int count = gameContext.getRailroadCount(*getOwner());
    if (rentTable.find(count) != rentTable.end()) {
        return rentTable.at(count);
    }
    return 0;
}

RailroadTile* RailroadTile::asRailroadTile() {
    return this;
}

const RailroadTile* RailroadTile::asRailroadTile() const {
    return this;
}

int RailroadTile::getSellValueToBank() const {
    return getMortgageValue();
}

PropertyType RailroadTile::getPropertyType() const {
    return PropertyType::RAILROAD;
}
