#include "models/tiles/GoTile.hpp"
#include "models/Player.hpp"
#include "core/GameContext.hpp"
#include "core/TurnManager.hpp"
#include "utils/TextFormatter.hpp"
#include "utils/TransactionLogger.hpp"

GoTile::GoTile() : ActionTile(), salary(0) {}

GoTile::GoTile(int index, const std::string &code, const std::string &name, int salary)
    : ActionTile(index, code, name, TileCategory::DEFAULT), salary(salary) {}

void GoTile::onLanded(Player& player, GameContext& gameContext) {
    int beforeBalance = player.getBalance();
    awardSalary(player);
    gameContext.showMessage(
        "Kamu mendarat di GO. Mendapatkan gaji " + TextFormatter::formatMoney(salary) + ".");
    gameContext.showMessage(
        "Uang kamu: " + TextFormatter::formatMoney(beforeBalance) +
            " -> " + TextFormatter::formatMoney(player.getBalance()));
    if (gameContext.getLogger() != nullptr) {
        int currentTurn = 0;
        if (gameContext.getTurnManager() != nullptr) {
            currentTurn = gameContext.getTurnManager()->getCurrentTurn();
        }
        gameContext.getLogger()->log(
            currentTurn,
            player.getUsername(),
            "GO",
            "Mendarat di GO, mendapatkan gaji " + TextFormatter::formatMoney(salary));
    }
}

void GoTile::onPassed(Player& player, GameContext&) {
    awardSalary(player);
}

void GoTile::awardSalary(Player &player) {
    player += salary;
}

int GoTile::getSalary() const {
    return salary;
}
