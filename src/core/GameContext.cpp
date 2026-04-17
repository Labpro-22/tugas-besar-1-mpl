#include "core/GameContext.hpp"

GameContext::GameContext(
        Board* board,
        TurnManager* turnManager,
        AuctionManager* auctionManager,
        BankruptcyHandler* bankruptcyHandler,
        TransactionLogger* logger,
        Dice* dice,
        CardDeck<ActionCard>* chanceDeck,
        CardDeck<ActionCard>* communityDeck,
        CardDeck<SkillCard>* skillDeck
) 
    : board(board), 
    turnManager(turnManager),
    auctionManager(auctionManager),
    bankruptcyHandler(bankruptcyHandler),
    logger(logger),
    dice(dice),
    chanceDeck(chanceDeck),
    communityDeck(communityDeck),
    skillDeck(skillDeck) {}

Board* GameContext::getBoard() const {
    return board;
}

TurnManager* GameContext::getTurnManager() const {
    return turnManager;
}

AuctionManager* GameContext::getAuctionManager() const {
    return auctionManager;
}

BankruptcyHandler* GameContext::getBankruptcyHandler() const {
    return bankruptcyHandler;
}

TransactionLogger* GameContext::getLogger() const {
    return logger;
}

Dice* GameContext::getDice() const {
    return dice;
}

CardDeck<ActionCard>* GameContext::getChanceDeck() const {
    return chanceDeck;
}

CardDeck<ActionCard>* GameContext::getCommunityDeck() const {
    return communityDeck;
}

CardDeck<SkillCard>* GameContext::getSkillDeck() const {
    return skillDeck;
}