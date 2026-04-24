#include "core/AssetManager.hpp"

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

#include "core/Board.hpp"
#include "core/GameContext.hpp"
#include "core/GameIO.hpp"
#include "core/TurnManager.hpp"
#include "models/Player.hpp"
#include "models/tiles/PropertyTile.hpp"
#include "models/tiles/StreetTile.hpp"
#include "utils/OutputFormatter.hpp"
#include "utils/TransactionLogger.hpp"

namespace {
    std::string fitColumn(const std::string& text, int width) {
        if (static_cast<int>(text.size()) >= width) {
            return text.substr(0, width);
        }
        return text + std::string(width - text.size(), ' ');
    }

    int buildingSellValue(const StreetTile& street) {
        int level = street.getBuildingLevel();
        if (level <= 0) {
            return 0;
        }
        if (level == 5) {
            return ((4 * street.getHouseCost()) + street.getHotelCost()) / 2;
        }
        return (level * street.getHouseCost()) / 2;
    }

    std::vector<StreetTile*> getOwnedBuildingsInColorGroup(Board& board, const Player& player, const StreetTile& selected) {
        std::vector<StreetTile*> result;
        std::vector<StreetTile*> group = board.getProperties(selected.getColorGroup());
        for (StreetTile* street : group) {
            if (street == nullptr || !street->isOwnedBy(player) || street->getBuildingLevel() <= 0) {
                continue;
            }
            result.push_back(street);
        }
        return result;
    }

    bool canBuildEvenly(const Board& board, const Player& player, const StreetTile& target) {
        if (!board.hasMonopoly(player, target.getColorGroup())) {
            return false;
        }

        std::vector<StreetTile*> group = board.getProperties(target.getColorGroup());
        if (group.empty()) {
            return false;
        }

        int targetLevel = target.getBuildingLevel();
        if (targetLevel >= 5) {
            return false;
        }

        int minLevel = 5;
        int maxLevel = 0;
        for (StreetTile* street : group) {
            if (street == nullptr || street->getStatus() != PropertyStatus::OWNED ||
                !street->isOwnedBy(player)) {
                return false;
            }

            minLevel = std::min(minLevel, street->getBuildingLevel());
            maxLevel = std::max(maxLevel, street->getBuildingLevel());
        }

        if (targetLevel < 4) {
            return targetLevel == minLevel;
        }

        return targetLevel == 4 && minLevel >= 4 && maxLevel <= 4;
    }

    void logAssetAction(
        GameContext& context,
        const Player& player,
        const std::string& action,
        const std::string& detail
    ) {
        TransactionLogger* logger = context.getLogger();
        TurnManager* turnManager = context.getTurnManager();
        if (logger != nullptr && turnManager != nullptr) {
            logger->log(turnManager->getCurrentTurn(), player.getUsername(), action, detail);
        }
    }

}

void AssetManager::mortgageProperty(Player& player, GameContext& context) {
    GameIO* io = context.getIO();
    Board* board = context.getBoard();
    if (io == nullptr || board == nullptr) {
        return;
    }

    std::vector<PropertyTile*> available;
    for (PropertyTile* property : player.getProperties()) {
        if (property != nullptr && property->getStatus() == PropertyStatus::OWNED) {
            available.push_back(property);
        }
    }

    if (available.empty()) {
        io->showMessage("Tidak ada properti yang dapat digadaikan.");
        return;
    }

    std::vector<int> validTileIndices;
    validTileIndices.reserve(available.size());
    for (PropertyTile* property : available) {
        if (property != nullptr) {
            validTileIndices.push_back(property->getIndex());
        }
    }

    const int selectedTileIndex = io->promptTileSelection(
        "Pilih properti yang ingin digadaikan langsung dari board.",
        validTileIndices,
        true
    );
    if (selectedTileIndex < 0) {
        return;
    }

    PropertyTile* selected = dynamic_cast<PropertyTile*>(board->getTile(selectedTileIndex));
    if (selected == nullptr || selected->getStatus() != PropertyStatus::OWNED || !selected->isOwnedBy(player)) {
        io->showMessage("Properti yang dipilih tidak valid untuk digadaikan.");
        return;
    }

    if (selected->getPropertyType() == PropertyType::STREET) {
        StreetTile* selectedStreet = selected->asStreetTile();
        if (selectedStreet == nullptr) {
            return;
        }

        std::vector<StreetTile*> buildings = getOwnedBuildingsInColorGroup(*board, player, *selectedStreet);
        if (!buildings.empty()) {
            const std::string groupLabel = OutputFormatter::formatColorGroup(selectedStreet->getColorGroup());
            io->showMessage(selected->getName() + " tidak dapat digadaikan!");
            io->showMessage("Masih terdapat bangunan di color group [" + groupLabel + "].");
            io->showMessage("Bangunan harus dijual terlebih dahulu.");
            io->showMessage("");
            io->showMessage("Daftar bangunan di color group [" + groupLabel + "]:");

            for (int i = 0; i < static_cast<int>(buildings.size()); ++i) {
                std::ostringstream line;
                std::string name = buildings[i]->getName() + " (" + buildings[i]->getCode() + ")";
                line << (i + 1) << ". "
                     << fitColumn(name, 24)
                     << " - " << fitColumn(OutputFormatter::formatBuildingLabel(*buildings[i]), 8)
                     << " -> Nilai jual bangunan: " << OutputFormatter::formatMoney(buildingSellValue(*buildings[i]));
                io->showMessage(line.str());
            }
            io->showMessage("");

            if (!io->confirmYN("Jual semua bangunan color group [" + groupLabel + "]?")) {
                return;
            }

            int totalReceived = 0;
            for (StreetTile* street : buildings) {
                int received = 0;
                while (street->getBuildingLevel() > 0) {
                    received += street->sellBuilding();
                }

                if (received > 0) {
                    totalReceived += received;
                    player += received;
                    io->showMessage(
                        "Bangunan " + street->getName() + " terjual. Kamu menerima " +
                        OutputFormatter::formatMoney(received) + "."
                    );
                }
            }

            io->showMessage("Uang kamu sekarang: " + OutputFormatter::formatMoney(player.getBalance()));
            io->showMessage("");

            if (!io->confirmYN("Lanjut menggadaikan " + selected->getName() + "?")) {
                return;
            }
        }
    }

    selected->mortgage();
    player += selected->getMortgageValue();
    io->showMessage(selected->getName() + " berhasil digadaikan.");
    io->showMessage("Kamu menerima " + OutputFormatter::formatMoney(selected->getMortgageValue()) + " dari Bank.");
    io->showMessage("Uang kamu sekarang: " + OutputFormatter::formatMoney(player.getBalance()));
    io->showMessage("Catatan: Sewa tidak dapat dipungut dari properti yang digadaikan.");
    logAssetAction(
        context,
        player,
        "GADAI",
        "Gadai " + selected->getName() + " (" + selected->getCode() +
            "), terima " + OutputFormatter::formatMoney(selected->getMortgageValue()));
}

void AssetManager::redeemProperty(Player& player, GameContext& context) {
    GameIO* io = context.getIO();
    if (io == nullptr) {
        return;
    }

    std::vector<PropertyTile*> mortgaged;
    for (PropertyTile* property : player.getProperties()) {
        if (property != nullptr && property->getStatus() == PropertyStatus::MORTGAGED) {
            mortgaged.push_back(property);
        }
    }

    if (mortgaged.empty()) {
        io->showMessage("Tidak ada properti yang sedang digadaikan.");
        return;
    }

    Board* board = context.getBoard();
    if (board == nullptr) {
        return;
    }

    io->showMessage("Uang kamu saat ini: " + OutputFormatter::formatMoney(player.getBalance()));

    std::vector<int> validTileIndices;
    validTileIndices.reserve(mortgaged.size());
    for (PropertyTile* property : mortgaged) {
        if (property != nullptr) {
            validTileIndices.push_back(property->getIndex());
        }
    }

    const int selectedTileIndex = io->promptTileSelection(
        "Pilih properti yang ingin di-unmortgage langsung dari board.",
        validTileIndices,
        true
    );
    if (selectedTileIndex < 0) {
        return;
    }

    PropertyTile* selected = dynamic_cast<PropertyTile*>(board->getTile(selectedTileIndex));
    if (selected == nullptr || selected->getStatus() != PropertyStatus::MORTGAGED || !selected->isOwnedBy(player)) {
        io->showMessage("Properti yang dipilih tidak valid untuk di-unmortgage.");
        return;
    }

    int originalCost = selected->getBuyPrice();
    int redeemCost = player.getDiscountedAmount(originalCost);
    if (!player.canAfford(redeemCost)) {
        io->showMessage(
            "Uang kamu tidak cukup untuk menebus " + selected->getName() + ".");
        io->showMessage(
            "Harga tebus: " + OutputFormatter::formatMoney(redeemCost) +
            " | Uang kamu: " + OutputFormatter::formatMoney(player.getBalance()));
        return;
    }

    redeemCost = player.consumeDiscountedAmount(originalCost);
    if (redeemCost != originalCost) {
        io->showMessage(
            "Diskon diterapkan dari " + OutputFormatter::formatMoney(originalCost) +
                " menjadi " + OutputFormatter::formatMoney(redeemCost) + ".");
    }

    player -= redeemCost;
    selected->redeem();
    io->showMessage("");
    io->showMessage(selected->getName() + " berhasil ditebus!");
    io->showMessage("Kamu membayar " + OutputFormatter::formatMoney(redeemCost) + " ke Bank.");
    io->showMessage("Uang kamu sekarang: " + OutputFormatter::formatMoney(player.getBalance()));
    logAssetAction(
        context,
        player,
        "TEBUS",
        "Tebus " + selected->getName() + " (" + selected->getCode() +
            "), bayar " + OutputFormatter::formatMoney(redeemCost));
}

void AssetManager::buildProperty(Player& player, GameContext& context) {
    GameIO* io = context.getIO();
    Board* board = context.getBoard();
    if (io == nullptr || board == nullptr) {
        return;
    }

    std::vector<StreetTile*> selectable;
    for (PropertyTile* property : player.getProperties()) {
        StreetTile* street = property == nullptr ? nullptr : property->asStreetTile();
        if (street == nullptr) {
            continue;
        }
        if (street->getStatus() != PropertyStatus::OWNED || !street->isOwnedBy(player)) {
            continue;
        }
        if (street->canBuildNext() && canBuildEvenly(*board, player, *street)) {
            selectable.push_back(street);
        }
    }

    if (selectable.empty()) {
        io->showMessage("Tidak ada properti yang memenuhi syarat untuk dibangun.");
        io->showMessage("Kamu harus memiliki seluruh petak dalam satu color group terlebih dahulu.");
        return;
    }

    std::vector<int> validTileIndices;
    validTileIndices.reserve(selectable.size());
    for (StreetTile* street : selectable) {
        if (street != nullptr) {
            validTileIndices.push_back(street->getIndex());
        }
    }

    io->showMessage("Uang kamu saat ini : " + OutputFormatter::formatMoney(player.getBalance()));
    const int selectedTileIndex = io->promptTileSelection(
        "Pilih properti yang ingin dibangun langsung dari board.",
        validTileIndices,
        true
    );
    if (selectedTileIndex < 0) {
        return;
    }

    StreetTile* selected = dynamic_cast<StreetTile*>(board->getTile(selectedTileIndex));
    if (selected == nullptr || selected->getStatus() != PropertyStatus::OWNED || !selected->isOwnedBy(player) ||
        !selected->canBuildNext() || !canBuildEvenly(*board, player, *selected)) {
        io->showMessage("Properti yang dipilih tidak valid untuk dibangun.");
        return;
    }
    int cost = selected->getBuildingLevel() == 4 ? selected->getHotelCost() : selected->getHouseCost();
    int costToPay = player.getDiscountedAmount(cost);

    if (selected->getBuildingLevel() == 4) {
        std::string prompt = "Upgrade ke hotel? Biaya: " + OutputFormatter::formatMoney(costToPay);
        if (costToPay != cost) {
            prompt += " (setelah diskon dari " + OutputFormatter::formatMoney(cost) + ")";
        }
        if (!io->confirmYN(prompt)) {
            return;
        }
    }

    if (!player.canAfford(costToPay)) {
        io->showMessage(
            "Uang kamu tidak cukup untuk membangun di " + selected->getName() + ".");
        io->showMessage(
            "Biaya: " + OutputFormatter::formatMoney(costToPay) +
            " | Uang kamu: " + OutputFormatter::formatMoney(player.getBalance()));
        return;
    }

    costToPay = player.consumeDiscountedAmount(cost);
    if (costToPay != cost) {
        io->showMessage(
            "Diskon diterapkan dari " + OutputFormatter::formatMoney(cost) +
                " menjadi " + OutputFormatter::formatMoney(costToPay) + ".");
    }

    player -= costToPay;
    selected->build();
    if (selected->getBuildingLevel() == 5) {
        io->showMessage(selected->getName() + " di-upgrade ke Hotel!");
    } else {
        io->showMessage(
            "Kamu membangun 1 rumah di " + selected->getName() +
            ". Biaya: " + OutputFormatter::formatMoney(costToPay));
    }
    io->showMessage("Uang tersisa: " + OutputFormatter::formatMoney(player.getBalance()));
    if (selected->getBuildingLevel() == 5) {
        logAssetAction(
            context,
            player,
            "BANGUN",
            "Upgrade " + selected->getName() + " (" + selected->getCode() +
                ") ke Hotel, biaya " + OutputFormatter::formatMoney(costToPay));
    } else {
        logAssetAction(
            context,
            player,
            "BANGUN",
            "Bangun 1 rumah di " + selected->getName() + " (" + selected->getCode() +
                "), biaya " + OutputFormatter::formatMoney(costToPay));
    }
}
