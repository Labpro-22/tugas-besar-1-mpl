#include "models/cards/TeleportCard.hpp"

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
    int targetIndex = -1;

    while (true) {
        std::cout << "Pilih index petak tujuan teleport (0-" << tileCount - 1 << "): ";
        if (std::cin >> targetIndex && targetIndex >= 0 && targetIndex < tileCount) {
            break;
        }

        std::cout << "Index tidak valid. Masukkan angka 0 sampai " << tileCount - 1 << ".\n";
        std::cin.clear();
    }

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