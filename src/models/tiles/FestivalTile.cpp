#include "models/tiles/FestivalTile.hpp"

#include <string>
#include <vector>
#include <algorithm>

#include "core/GameContext.hpp"
#include "core/GameIO.hpp"
#include "core/TurnManager.hpp"
#include "models/Player.hpp"
#include "models/tiles/PropertyTile.hpp"
#include "models/tiles/StreetTile.hpp"
#include "utils/TextFormatter.hpp"
#include "utils/TransactionLogger.hpp"

FestivalTile::FestivalTile() : ActionTile() {}

FestivalTile::FestivalTile(int index, const std::string& code, const std::string& name)
    : ActionTile(index, code, name) {}

void FestivalTile::onLanded(Player& player, GameContext& gameContext) {
    gameContext.showMessage("Kamu mendarat di petak Festival!");

    std::vector<StreetTile*> streets;
    getPlayerStreets(player, streets);
    if (streets.empty()) {
        gameContext.showMessage("Kamu belum memiliki lahan yang dapat diberi efek festival.");
        return;
    }

    if (!gameContext.hasIO()) {
        applyFestivalEffect(streets.front(), player, gameContext);
        return;
    }

    std::sort(streets.begin(), streets.end(), [](const StreetTile* lhs, const StreetTile* rhs) {
        return lhs->getIndex() < rhs->getIndex();
    });

    GameIO* io = gameContext.getIO();
    if (io->usesRichGuiPresentation()) {
        std::vector<int> validTileIndices;
        validTileIndices.reserve(streets.size());
        for (StreetTile* street : streets) {
            if (street != nullptr) {
                validTileIndices.push_back(street->getIndex());
            }
        }

        int selectedTileIndex = io->promptTileSelection(
            "Pilih lahan yang akan mendapat efek festival langsung dari board.",
            validTileIndices);

        for (StreetTile* street : streets) {
            if (street != nullptr && street->getIndex() == selectedTileIndex) {
                applyFestivalEffect(street, player, gameContext);
                return;
            }
        }
        return;
    }

    gameContext.showMessage("");
    gameContext.showMessage("Daftar properti milikmu:");
    for (StreetTile* street : streets) {
        gameContext.showMessage("- " + street->getCode() + " (" + street->getName() + ")");
    }

    while (io != nullptr) {
        std::string code = io->promptText("\nMasukkan kode properti: ");
        for (StreetTile* street : streets) {
            if (street != nullptr && street->getCode() == code) {
                applyFestivalEffect(street, player, gameContext);
                return;
            }
        }
        gameContext.showMessage("-> Kode properti tidak valid!");
    }
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

    int previousMultiplier = selectedStreet->getFestivalMultiplier();
    int previousDuration = selectedStreet->getFestivalDuration();
    int previousRent = selectedStreet->calculateRent(0, gameContext);

    selectedStreet->applyFestival();
    int currentRent = selectedStreet->calculateRent(0, gameContext);

    gameContext.showMessage("");
    if (previousDuration <= 0) {
        gameContext.showMessage("Efek festival aktif!");
        gameContext.showMessage("");
        gameContext.showMessage("Sewa awal: " + TextFormatter::formatMoney(previousRent));
        gameContext.showMessage("Sewa sekarang: " + TextFormatter::formatMoney(currentRent));
        gameContext.showMessage("Durasi: " + std::to_string(selectedStreet->getFestivalDuration()) + " giliran");
    } else if (previousMultiplier < 8) {
        gameContext.showMessage("Efek diperkuat!");
        gameContext.showMessage("");
        gameContext.showMessage("Sewa sebelumnya: " + TextFormatter::formatMoney(previousRent));
        gameContext.showMessage("Sewa sekarang: " + TextFormatter::formatMoney(currentRent));
        gameContext.showMessage("Durasi di-reset menjadi: " + std::to_string(selectedStreet->getFestivalDuration()) + " giliran");
    } else {
        gameContext.showMessage("Efek sudah maksimum (harga sewa sudah digandakan tiga kali)");
        gameContext.showMessage("");
        gameContext.showMessage("Durasi di-reset menjadi: " + std::to_string(selectedStreet->getFestivalDuration()) + " giliran");
    }

    if (gameContext.getLogger() != nullptr && gameContext.getTurnManager() != nullptr) {
        int currentTurn = gameContext.getTurnManager()->getCurrentTurn();
        gameContext.getLogger()->log(
            currentTurn,
            player.getUsername(),
            "FESTIVAL",
            selectedStreet->getName() + " (" + selectedStreet->getCode() + "): sewa " +
            TextFormatter::formatMoney(previousRent) + " -> " + TextFormatter::formatMoney(currentRent) +
            ", durasi " + std::to_string(selectedStreet->getFestivalDuration()) + " giliran");
    }
}
