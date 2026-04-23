#include "models/tiles/UtilityTile.hpp"
#include "core/GameContext.hpp"
#include "models/Player.hpp"

UtilityTile::UtilityTile() : PropertyTile() {}

UtilityTile::UtilityTile(
    int index, const std::string& code, const std::string& name, 
    int mortgageValue, const std::map<int, int>& multiplierTable
) : PropertyTile(index, code, name, 0, mortgageValue), multiplierTable(multiplierTable) {}

void UtilityTile::onLanded(Player& player, GameContext& gameContext) {
    if (getStatus() == PropertyStatus::BANK) {
        transferTo(player);
        gameContext.showMessage("Kamu mendarat di " + getName() + "!");
        gameContext.showMessage("Belum ada yang menginjaknya duluan, " + getName() + " kini menjadi milikmu!");
        gameContext.logEvent(
            "UTILITY",
            getName() + " kini milik " + player.getUsername() + " (otomatis)");
    } else {
        gameContext.triggerRentEvent(player, *this);
    }
}

int UtilityTile::calculateRent(int diceTotal, const GameContext& gameContext) {
    if (getStatus() != PropertyStatus::OWNED) {
        return 0;
    }
    int count = gameContext.getUtilityCount(*getOwner());
    if (multiplierTable.find(count) != multiplierTable.end()) {
        return diceTotal * multiplierTable.at(count);
    }
    return 0;
}

int UtilityTile::getSellValueToBank() const {
    return 0;
}

PropertyType UtilityTile::getPropertyType() const {
    return PropertyType::UTILITY;
}
