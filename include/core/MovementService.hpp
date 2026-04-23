#pragma once

class Board;
class GameContext;
class Player;
class Tile;

class MovementService {
public:
    static void awardGoSalaryForForwardMovement(
        Board& board,
        Player& player,
        GameContext& gameContext,
        int oldPosition,
        int targetIndex
    );

    static bool shouldSkipGoLandingSalary(const Tile* tile);
};
