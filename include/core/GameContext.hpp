#pragma once

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
        AuctionManager* AuctionManager;
        BankruptcyHandler* bankruptcyHandler;
        TransactionLogger* logger;
        Dice* dice;
        CardDeck<ActionCard>* chanceDeck;
        CardDeck<ActionCard>* communityDeck;
        CardDeck<SkillCard>* skillDeck;

    public:
        GameContext(Board* board,
                    TurnManager* turnManager,
                    AuctionManager* auctionManager,
                    BankruptcyHandler* bankruptcyHandler,
                    TransactionLogger* logger,
                    Dice* dice,
                    CardDeck<ActionCard>* chanceDeck,
                    CardDeck<ActionCard>* communityDeck,
                    CardDeck<SkillCard>* skillDeck);
        Board* getBoard() const;
        TurnManager* getTurnManager() const;
        AuctionManager* getAuctionManager() const;
        BankruptcyHandler* getBankruptcyHandler() const;
        TransactionLogger* getLogger() const;
        Dice* getDice() const;
        CardDeck<ActionCard>* getChanceDeck() const;
        CardDeck<ActionCard>* getCommunityDeck() const;
        CardDeck<SkillCard>* getSkillDeck() const;
};