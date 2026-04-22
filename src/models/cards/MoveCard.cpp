#include "models/cards/MoveCard.hpp"

#include "core/Board.hpp"
#include "core/GameContext.hpp"
#include "models/Player.hpp"
#include "models/tiles/GoTile.hpp"
#include "models/tiles/Tile.hpp"

MoveCard::MoveCard()
    : SkillCard() {}

MoveCard::MoveCard(int value, int remainingDuration)
    : SkillCard(value, remainingDuration) {}

std::string MoveCard::getTypeName() const {
    return "MoveCard";
}

void MoveCard::use(Player& player, GameContext& gameContext) {
    Board* board = gameContext.getBoard();
    if (board == nullptr || board->getTileCount() <= 0) {
        return;
    }

    int tileCount = board->getTileCount();
    int targetIndex = (player.getPosition() + getValue()) % tileCount;
    bool passedGo = player.moveTo(targetIndex);
    player.setUsedSkillThisTurn(true);

    if (passedGo) {
        GoTile* goTile = dynamic_cast<GoTile*>(board->getTile("GO"));
        if (goTile != nullptr) {
            goTile->awardSalary(player);
        }
    }

    Tile* targetTile = board->getTile(targetIndex);
    if (targetTile != nullptr) {
        targetTile->onLanded(player, gameContext);
    }
}
