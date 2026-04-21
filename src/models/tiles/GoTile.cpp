#include "models/tiles/GoTile.hpp"
#include "models/Player.hpp"
#include "core/GameContext.hpp"
#include "core/TurnManager.hpp"
#include "utils/TransactionLogger.hpp"

GoTile::GoTile() : ActionTile(), salary(0) {}

GoTile::GoTile(int index, const std::string &code, const std::string &name, int salary)
    : ActionTile(index, code, name, TileCategory::DEFAULT), salary(salary) {}

void GoTile::onLanded(Player& player, GameContext& gameContext) {
    // Gajian saat mendarat
    awardSalary(player);
    int currentTurn = gameContext.getTurnManager()->getCurrentTurn();
    gameContext.getLogger()->log(currentTurn, player.getUsername(), "GO", "Mendarat di GO, mendapatkan gaji " + std::to_string(salary));
}

void GoTile::awardSalary(Player &player) {
    player += salary; // Transfer gaji pake operator overloading += dari Player
}

std::string GoTile::getDisplayLabel() const {
    return getCode(); // Buat "GO"
}

int GoTile::getSalary() const {
    return salary;
}
