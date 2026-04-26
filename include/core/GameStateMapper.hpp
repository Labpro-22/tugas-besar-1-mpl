#pragma once

#include <vector>

class Board;
class ConfigData;
class GameState;
class Player;
class SkillCard;
class TransactionLogger;
class TurnManager;

template <typename T>
class CardDeck;

class GameStateMapper {
public:
    static GameState create(
        const Board& board,
        const std::vector<Player>& players,
        const TurnManager& turnManager,
        const CardDeck<SkillCard>& skillDeck,
        const ConfigData& configData,
        TransactionLogger* logger
    );
};
