#pragma once

#include <string>
#include "models/Player.hpp"

class Board;
class TurnManager;
class AuctionManager;
class TransactionLogger;
class Dice;
class ActionCard;
class SkillCard;
class BankruptcyHandler;
template <typename T> class CardDeck;

class GameContext {
private:
    Board* board;
    TurnManager* turnManager;
    AuctionManager* auctionManager;
    BankruptcyHandler* bankruptcyHandler;
    TransactionLogger* logger;
    Dice* dice;
    CardDeck<ActionCard>* chanceDeck;
    CardDeck<ActionCard>* communityDeck;
    CardDeck<SkillCard>* skillDeck;

public:
    GameContext(
        Board* board,
        TurnManager* turnManager,
        AuctionManager* auctionManager,
        BankruptcyHandler* bankruptcyHandler,
        TransactionLogger* logger,
        Dice* dice,
        CardDeck<ActionCard>* chanceDeck,
        CardDeck<ActionCard>* communityDeck,
        CardDeck<SkillCard>* skillDeck
    );

    Board* getBoard() const;
    TurnManager* getTurnManager() const;
    AuctionManager* getAuctionManager() const;
    BankruptcyHandler* getBankruptcyHandler() const;
    TransactionLogger* getLogger() const;
    Dice* getDice() const;
    CardDeck<ActionCard>* getChanceDeck() const;
    CardDeck<ActionCard>* getCommunityDeck() const;
    CardDeck<SkillCard>* getSkillDeck() const;

    void triggerStreetEvent(Player& player, PropertyTile& tile);
    void triggerRentEvent(Player& player, PropertyTile& tile);
    bool hasMonopoly(const Player& player, ColorGroup colorGroup) const;
    int getRailroadCount(const Player& player) const;
    int getUtilityCount(const Player& player) const;
    void logEvent(const std::string& actionType, const std::string& detail);
};
