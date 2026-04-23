#include "core/AssetManager.hpp"

#include <algorithm>
#include <string>
#include <vector>

#include "core/Board.hpp"
#include "core/GameContext.hpp"
#include "core/GameIO.hpp"
#include "core/TurnManager.hpp"
#include "models/Player.hpp"
#include "models/tiles/PropertyTile.hpp"
#include "models/tiles/StreetTile.hpp"
#include "utils/TransactionLogger.hpp"

namespace {
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

    int sellBuildingsInColorGroup(Board& board, const Player& player, StreetTile& selected) {
        int totalReceived = 0;
        std::vector<StreetTile*> group = board.getProperties(selected.getColorGroup());
        for (StreetTile* street : group) {
            if (street == nullptr || !street->isOwnedBy(player)) {
                continue;
            }

            while (street->getBuildingLevel() > 0) {
                totalReceived += street->sellBuilding();
            }
        }

        return totalReceived;
    }

    void logAssetAction(GameContext& context, const Player& player, const std::string& action, const std::string& code) {
        TransactionLogger* logger = context.getLogger();
        TurnManager* turnManager = context.getTurnManager();
        if (logger != nullptr && turnManager != nullptr) {
            logger->log(turnManager->getCurrentTurn(), player.getUsername(), action, code);
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

    int selectedTileIndex = io->promptTileSelection(
        "Pilih properti milikmu yang ingin digadaikan langsung dari board.",
        validTileIndices);

    PropertyTile* selected = nullptr;
    for (PropertyTile* property : available) {
        if (property != nullptr && property->getIndex() == selectedTileIndex) {
            selected = property;
            break;
        }
    }

    if (selected == nullptr) {
        return;
    }

    if (selected->getPropertyType() == PropertyType::STREET) {
        StreetTile* selectedStreet = selected->asStreetTile();
        if (selectedStreet == nullptr) {
            return;
        }
        int received = sellBuildingsInColorGroup(*board, player, *selectedStreet);
        if (received > 0) {
            player += received;
            io->showMessage(
                "Bangunan pada color group " + selectedStreet->getCode() +
                    " dijual terlebih dahulu. Kamu menerima M" + std::to_string(received) + ".");
        }
    }

    selected->mortgage();
    player += selected->getMortgageValue();
    io->showMessage(
        selected->getName() + " berhasil digadaikan. Kamu menerima M" +
            std::to_string(selected->getMortgageValue()) + ".");
    logAssetAction(context, player, "GADAI", selected->getCode());
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
        io->showMessage("Tidak ada properti tergadai.");
        return;
    }

    for (int i = 0; i < static_cast<int>(mortgaged.size()); ++i) {
        int redeemCost = player.getDiscountedAmount(mortgaged[i]->getBuyPrice());
        io->showMessage(
            std::to_string(i + 1) + ". " + mortgaged[i]->getName()
                + " (" + mortgaged[i]->getCode() + ") - Tebus M"
                + std::to_string(redeemCost));
    }

    int choice = io->promptIntInRange("Pilih properti: ", 1, static_cast<int>(mortgaged.size()));
    PropertyTile* selected = mortgaged[choice - 1];
    int originalCost = selected->getBuyPrice();
    int redeemCost = player.getDiscountedAmount(originalCost);
    if (!player.canAfford(redeemCost)) {
        player -= redeemCost;
    }
    redeemCost = player.consumeDiscountedAmount(originalCost);
    if (redeemCost != originalCost) {
        io->showMessage(
            "Diskon diterapkan dari M" + std::to_string(originalCost) +
                " menjadi M" + std::to_string(redeemCost) + ".");
    }

    player -= redeemCost;
    selected->redeem();
    io->showPaymentNotification(
        "PAYMENT",
        player.getUsername() + " membayar M" + std::to_string(selected->getBuyPrice()) +
            " untuk unmortgage " + selected->getName() + ".");
    io->showMessage(
        selected->getName() + " berhasil ditebus dengan M" +
            std::to_string(redeemCost) + ".");
    logAssetAction(context, player, "TEBUS", selected->getCode());
}

void AssetManager::buildProperty(Player& player, GameContext& context) {
    GameIO* io = context.getIO();
    Board* board = context.getBoard();
    if (io == nullptr || board == nullptr) {
        return;
    }

    std::vector<StreetTile*> buildable;
    for (PropertyTile* property : player.getProperties()) {
        if (property == nullptr || property->getPropertyType() != PropertyType::STREET) {
            continue;
        }

        StreetTile* street = property->asStreetTile();
        if (street == nullptr) {
            continue;
        }
        if (street->getStatus() == PropertyStatus::OWNED &&
            street->canBuildNext() && canBuildEvenly(*board, player, *street)) {
            buildable.push_back(street);
        }
    }

    if (buildable.empty()) {
        io->showMessage("Tidak ada properti yang bisa dibangun.");
        return;
    }

    for (int i = 0; i < static_cast<int>(buildable.size()); ++i) {
        int cost = buildable[i]->getBuildingLevel() == 4
            ? buildable[i]->getHotelCost()
            : buildable[i]->getHouseCost();
        int costToPay = player.getDiscountedAmount(cost);
        io->showMessage(
            std::to_string(i + 1) + ". " + buildable[i]->getName()
                + " (" + buildable[i]->getCode() + ") - Biaya M"
                + std::to_string(costToPay));
    }

    int choice = io->promptIntInRange("Pilih properti: ", 1, static_cast<int>(buildable.size()));
    StreetTile* selected = buildable[choice - 1];
    int cost = selected->getBuildingLevel() == 4
        ? selected->getHotelCost()
        : selected->getHouseCost();
    int costToPay = player.getDiscountedAmount(cost);
    if (!player.canAfford(costToPay)) {
        player -= costToPay;
    }
    costToPay = player.consumeDiscountedAmount(cost);
    if (costToPay != cost) {
        io->showMessage(
            "Diskon diterapkan dari M" + std::to_string(cost) +
                " menjadi M" + std::to_string(costToPay) + ".");
    }

    player -= costToPay;
    selected->build();
    io->showPaymentNotification(
        "PAYMENT",
        player.getUsername() + " membayar M" + std::to_string(cost) +
            " untuk membangun di " + selected->getName() + ".");
    io->showMessage(
        "Pembangunan berhasil di " + selected->getName() +
            ". Level bangunan sekarang: " + std::to_string(selected->getBuildingLevel()) + ".");
    logAssetAction(context, player, "BANGUN", selected->getCode());
}
