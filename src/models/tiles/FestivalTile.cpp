#include "models/tiles/FestivalTile.hpp"

#include <string>
#include <vector>

#include "core/GameContext.hpp"
#include "core/GameIO.hpp"
#include "core/TurnManager.hpp"
#include "models/Player.hpp"
#include "models/tiles/PropertyTile.hpp"
#include "models/tiles/StreetTile.hpp"
#include "utils/TransactionLogger.hpp"

FestivalTile::FestivalTile() : ActionTile() {}

FestivalTile::FestivalTile(int index, const std::string& code, const std::string& name)
    : ActionTile(index, code, name, TileCategory::DEFAULT) {}

void FestivalTile::onLanded(Player& player, GameContext& gameContext) {
    GameIO* io = gameContext.getIO();
    if (io != nullptr) {
        io->showMessage("Kamu mendarat di petak Festival!");
    }

    std::vector<StreetTile*> streets;
    getPlayerStreets(player, streets);
    if (streets.empty()) {
        if (io != nullptr) {
            io->showMessage("Kamu belum memiliki lahan yang dapat diberi efek festival.");
        }
        return;
    }

    if (io == nullptr) {
        applyFestivalEffect(streets.front(), player, gameContext);
        return;
    }

    io->showMessage("Pilih lahan yang akan mendapat efek festival:");
    for (int i = 0; i < static_cast<int>(streets.size()); ++i) {
        io->showMessage(
            std::to_string(i + 1) + ". " + streets[i]->getName() +
                " (" + streets[i]->getCode() + ")");
    }

    int choice = io->promptIntInRange("Pilih lahan: ", 1, static_cast<int>(streets.size()));
    applyFestivalEffect(streets[choice - 1], player, gameContext);
}

void FestivalTile::getPlayerStreets(const Player& player, std::vector<StreetTile*>& outStreets) const {
    for (PropertyTile* property : player.getProperties()) {
        if (property != nullptr &&
            property->getStatus() == PropertyStatus::OWNED &&
            property->asStreetTile() != nullptr) {
            outStreets.push_back(property->asStreetTile());
        }
    }
}

void FestivalTile::applyFestivalEffect(StreetTile* selectedStreet, Player& player, GameContext& gameContext) const {
    if (selectedStreet == nullptr) {
        return;
    }

    selectedStreet->applyFestival();
    if (gameContext.getIO() != nullptr) {
        gameContext.getIO()->showMessage(
            selectedStreet->getName() + " (" + selectedStreet->getCode() +
                "): sewa dikalikan " + std::to_string(selectedStreet->getFestivalMultiplier()) +
                ", durasi " + std::to_string(selectedStreet->getFestivalDuration()) + " giliran.");
    }

    if (gameContext.getLogger() != nullptr && gameContext.getTurnManager() != nullptr) {
        int currentTurn = gameContext.getTurnManager()->getCurrentTurn();
        gameContext.getLogger()->log(
            currentTurn,
            player.getUsername(),
            "FESTIVAL",
            "Mengaktifkan festival pada " + selectedStreet->getName());
    }
}

std::string FestivalTile::getDisplayLabel() const {
    return getCode();
}
