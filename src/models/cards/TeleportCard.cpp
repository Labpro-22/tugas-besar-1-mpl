#include "models/cards/TeleportCard.hpp"

#include <iostream>
#include <limits>

#include "core/Board.hpp"
#include "core/GameContext.hpp"
#include "models/Player.hpp"
#include "models/tiles/Tile.hpp"

TeleportCard::TeleportCard()
    : SkillCard() {}

std::string TeleportCard::getTypeName() const {
    return "TeleportCard";
}

void TeleportCard::use(Player& player, GameContext& gameContext) {
    Board* board = gameContext.getBoard();
    if (board == nullptr || board->getTileCount() <= 0) {
        return;
    }

    int tileCount = board->getTileCount();
    int targetPosition = -1;

    while (true) {
        std::cout << "Pilih nomor petak tujuan teleport (1-" << tileCount << "): ";
        if (std::cin >> targetPosition && targetPosition >= 1 && targetPosition <= tileCount) {
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            break;
        }

        std::cout << "Nomor petak tidak valid. Masukkan angka 1 sampai " << tileCount << ".\n";
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }

    int targetIndex = targetPosition - 1;
    Tile* targetTile = board->getTile(targetIndex);
    if (targetTile == nullptr) {
        return;
    }

    player.moveTo(targetIndex);
    player.setUsedSkillThisTurn(true);

    std::cout << "Teleport berhasil! " << player.getUsername()
              << " berpindah ke " << targetTile->getName()
              << " (" << targetTile->getCode() << ").\n";

    targetTile->onLanded(player, gameContext);
}
