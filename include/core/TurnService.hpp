#pragma once

class Board;
class ConfigData;
class GameIO;
class Player;
class SkillCard;
class TransactionLogger;
class TurnManager;

template <typename T>
class CardDeck;

class TurnService {
public:
    static void processTurn(
        Player& player,
        Board& board,
        CardDeck<SkillCard>& skillDeck,
        const ConfigData& configData,
        GameIO& io,
        TurnManager& turnManager,
        TransactionLogger* logger
    );
};
