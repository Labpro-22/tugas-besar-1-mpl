#include "models/tiles/FestivalTile.hpp"
#include "models/Player.hpp"
#include "core/GameContext.hpp"
#include "models/tiles/StreetTile.hpp"
#include "utils/TransactionLogger.hpp"
#include "core/TurnManager.hpp"

FestivalTile::FestivalTile() : ActionTile() {}

FestivalTile::FestivalTile(int index, const std::string& code, const std::string& name)
    : ActionTile(index, code, name, TileCategory::DEFAULT) {}

void FestivalTile::onLanded(Player& player, GameContext& gameContext) {
    // NOTE: Tile hanya handle logic, bukan output
    // GameEngine akan manage display info dan user input via command handler
    // Untuk sekarang: tile hanya check apakah ada properti street yang bisa di-festival
    // Jika tidak ada, tidak perlu action tambahan
}

void FestivalTile::getPlayerStreets(const Player& player, std::vector<StreetTile*>& outStreets) const {
    for (auto* p : player.getProperties()) {
        StreetTile* s = dynamic_cast<StreetTile*>(p);
        if (s) {
            outStreets.push_back(s);
        }
    }
}

void FestivalTile::applyFestivalEffect(StreetTile* selectedStreet, Player& player, GameContext& gameContext) const {
    if (!selectedStreet) {
        return;
    }

    selectedStreet->applyFestival();
    int currentTurn = gameContext.getTurnManager()->getCurrentTurn();
    gameContext.getLogger()->log(currentTurn, player.getUsername(), "FESTIVAL", 
        "Mengaktifkan festival pada " + selectedStreet->getName());
}

std::string FestivalTile::getDisplayLabel() const { return getCode(); }