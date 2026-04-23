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

    std::vector<StreetTile*> getOwnedStreetsInColorGroup(Board& board, const Player& player, ColorGroup colorGroup) {
        std::vector<StreetTile*> result;
        std::vector<StreetTile*> group = board.getProperties(colorGroup);
        for (StreetTile* street : group) {
            if (street == nullptr || !street->isOwnedBy(player) || street->getStatus() != PropertyStatus::OWNED) {
                continue;
            }
            result.push_back(street);
        }
        std::sort(result.begin(), result.end(), [](const StreetTile* lhs, const StreetTile* rhs) {
            return lhs->getIndex() < rhs->getIndex();
        });
        return result;
    }

    std::vector<ColorGroup> getBuildableColorGroups(Board& board, const Player& player) {
        std::vector<ColorGroup> groups;
        const ColorGroup streetGroups[] = {
            ColorGroup::COKLAT,
            ColorGroup::BIRU_MUDA,
            ColorGroup::MERAH_MUDA,
            ColorGroup::ORANGE,
            ColorGroup::MERAH,
            ColorGroup::KUNING,
            ColorGroup::HIJAU,
            ColorGroup::BIRU_TUA
        };

        for (ColorGroup colorGroup : streetGroups) {
            if (!board.hasMonopoly(player, colorGroup)) {
                continue;
            }

            std::vector<StreetTile*> streets = getOwnedStreetsInColorGroup(board, player, colorGroup);
            bool hasBuildableStreet = false;
            for (StreetTile* street : streets) {
                if (street != nullptr && street->canBuildNext()) {
                    hasBuildableStreet = true;
                    break;
                }
            }

            if (hasBuildableStreet) {
                groups.push_back(colorGroup);
            }
        }

        return groups;
    }

    void printGroupBuildSummary(GameIO* io, Board& board, const Player& player, ColorGroup colorGroup) {
        std::vector<StreetTile*> streets = getOwnedStreetsInColorGroup(board, player, colorGroup);
        for (StreetTile* street : streets) {
            int cost = street->getBuildingLevel() == 4 ? street->getHotelCost() : street->getHouseCost();
            std::ostringstream line;
            line << "   - " << std::left << std::setw(26)
                 << (street->getName() + " (" + street->getCode() + ")")
                 << " : " << OutputFormatter::formatBuildingLabel(*street);
            if (street->getBuildingLevel() < 5) {
                line << " (Harga " << (street->getBuildingLevel() == 4 ? "hotel" : "rumah")
                     << ": " << OutputFormatter::formatMoney(cost) << ")";
            }
            io->showMessage(line.str());
        }
    }

    void printGroupBuildState(GameIO* io,
                              Board& board,
                              const Player& player,
                              ColorGroup colorGroup,
                              const std::vector<StreetTile*>& selectable) {
        std::vector<StreetTile*> streets = getOwnedStreetsInColorGroup(board, player, colorGroup);
        for (StreetTile* street : streets) {
            bool isSelectable = std::find(selectable.begin(), selectable.end(), street) != selectable.end();
            std::ostringstream line;
            line << "- " << std::left << std::setw(26)
                 << (street->getName() + " (" + street->getCode() + ")")
                 << " : " << std::setw(8) << OutputFormatter::formatBuildingLabel(*street);

            if (street->getBuildingLevel() == 5) {
                line << " <- sudah maksimal, tidak dapat dibangun";
            } else if (street->getBuildingLevel() == 4) {
                if (isSelectable) {
                    line << " <- siap upgrade ke hotel";
                }
            } else if (isSelectable) {
                line << " <- dapat dibangun";
            }

            io->showMessage(line.str());
        }
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

    io->showMessage("=== Properti yang Dapat Digadaikan ===");
    for (int i = 0; i < static_cast<int>(available.size()); ++i) {
        std::ostringstream line;
        std::string name = available[i]->getName() + " (" + available[i]->getCode() + ")";
        std::string category = "[" + OutputFormatter::formatPropertyCategory(*available[i]) + "]";
        line << (i + 1) << ". "
             << std::left << std::setw(21) << name
             << std::setw(11) << category
             << " Nilai Gadai: " << OutputFormatter::formatMoney(available[i]->getMortgageValue());
        io->showMessage(line.str());
    }

    io->showMessage("");
    int choice = io->promptIntInRange(
        "Pilih nomor properti (0 untuk batal): ",
        0,
        static_cast<int>(available.size()));
    if (choice == 0) {
        return;
    }
    PropertyTile* selected = available[choice - 1];

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
                     << std::left << std::setw(24) << name
                     << " - " << std::setw(8) << OutputFormatter::formatBuildingLabel(*buildings[i])
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

    io->showMessage("=== Properti yang Sedang Digadaikan ===");
    for (int i = 0; i < static_cast<int>(mortgaged.size()); ++i) {
        int redeemCost = player.getDiscountedAmount(mortgaged[i]->getBuyPrice());
        std::ostringstream line;
        std::string name = mortgaged[i]->getName() + " (" + mortgaged[i]->getCode() + ")";
        std::string category = "[" + OutputFormatter::formatPropertyCategory(*mortgaged[i]) + "]";
        line << (i + 1) << ". "
             << std::left << std::setw(18) << name
             << std::setw(12) << category
             << "[M]  Harga Tebus: " << OutputFormatter::formatMoney(redeemCost);
        io->showMessage(line.str());
    }

    io->showMessage("");
    io->showMessage("Uang kamu saat ini: " + OutputFormatter::formatMoney(player.getBalance()));
    int choice = io->promptIntInRange(
        "Pilih nomor properti (0 untuk batal): ",
        0,
        static_cast<int>(mortgaged.size()));
    if (choice == 0) {
        return;
    }
    PropertyTile* selected = mortgaged[choice - 1];
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

    std::vector<ColorGroup> buildableGroups = getBuildableColorGroups(*board, player);
    if (buildableGroups.empty()) {
        io->showMessage("Tidak ada color group yang memenuhi syarat untuk dibangun.");
        io->showMessage("Kamu harus memiliki seluruh petak dalam satu color group terlebih dahulu.");
        return;
    }

    io->showMessage("=== Color Group yang Memenuhi Syarat ===");
    for (int i = 0; i < static_cast<int>(buildableGroups.size()); ++i) {
        io->showMessage(std::to_string(i + 1) + ". [" + OutputFormatter::formatColorGroup(buildableGroups[i]) + "]");
        printGroupBuildSummary(io, *board, player, buildableGroups[i]);
    }
    io->showMessage("");
    io->showMessage("Uang kamu saat ini : " + OutputFormatter::formatMoney(player.getBalance()));
    int groupChoice = io->promptIntInRange(
        "Pilih nomor color group (0 untuk batal): ",
        0,
        static_cast<int>(buildableGroups.size()));
    if (groupChoice == 0) {
        return;
    }

    ColorGroup selectedGroup = buildableGroups[groupChoice - 1];
    std::vector<StreetTile*> groupStreets = getOwnedStreetsInColorGroup(*board, player, selectedGroup);
    std::vector<StreetTile*> selectable;
    bool allFourHouses = !groupStreets.empty();
    for (StreetTile* street : groupStreets) {
        if (street == nullptr) {
            continue;
        }
        if (street->getBuildingLevel() != 4 && street->getBuildingLevel() != 5) {
            allFourHouses = false;
        }
        if (street->canBuildNext() && canBuildEvenly(*board, player, *street)) {
            selectable.push_back(street);
        }
    }

    io->showMessage("");
    io->showMessage("Color group [" + OutputFormatter::formatColorGroup(selectedGroup) + "]:");
    printGroupBuildState(io, *board, player, selectedGroup, selectable);
    io->showMessage("");

    if (allFourHouses) {
        io->showMessage(
            "Seluruh color group [" + OutputFormatter::formatColorGroup(selectedGroup) +
            "] sudah memiliki 4 rumah. Siap di-upgrade ke hotel!"
        );
    }

    if (selectable.empty()) {
        return;
    }

    for (int i = 0; i < static_cast<int>(selectable.size()); ++i) {
        io->showMessage(std::to_string(i + 1) + ". " + selectable[i]->getName() + " (" + selectable[i]->getCode() + ")");
    }
    int choice = io->promptIntInRange("Pilih petak (0 untuk batal): ", 0, static_cast<int>(selectable.size()));
    if (choice == 0) {
        return;
    }

    StreetTile* selected = selectable[choice - 1];
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
    std::vector<StreetTile*> updatedSelectable;
    std::vector<StreetTile*> updatedGroupStreets = getOwnedStreetsInColorGroup(*board, player, selectedGroup);
    for (StreetTile* street : updatedGroupStreets) {
        if (street != nullptr && street->canBuildNext() && canBuildEvenly(*board, player, *street)) {
            updatedSelectable.push_back(street);
        }
    }
    printGroupBuildState(io, *board, player, selectedGroup, updatedSelectable);
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
