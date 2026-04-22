#include "core/GameContext.hpp"
#include "core/Board.hpp"
#include "core/TurnManager.hpp"
#include "utils/TransactionLogger.hpp"

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

void GameContext::triggerStreetEvent(Player& player, PropertyTile& tile) {
    if (turnManager) {
        // Mendelegasikan alur beli/sewa/lelang lahan ke TurnManager
        turnManager->handlePropertyLanded(player, tile, *this); 
    }
}

void GameContext::triggerRentEvent(Player& player, PropertyTile& tile) {
    if (turnManager) {
        // Mendelegasikan alur bayar sewa khusus ke TurnManager
        turnManager->handleRentPayment(player, tile, *this);
    }
}

bool GameContext::hasMonopoly(const Player& player, ColorGroup colorGroup) const {
    if (board) {
        // Mendelegasikan pengecekan monopoli ke Board
        return board->hasMonopoly(player, colorGroup);
    }
    return false;
}

int GameContext::getRailroadCount(const Player& player) const {
    if (board) {
        // Mendelegasikan penghitungan stasiun ke Board
        return board->getRailroadCount(player);
    }
    return 0;
}

int GameContext::getUtilityCount(const Player& player) const {
    if (board) {
        // Mendelegasikan penghitungan utilitas ke Board
        return board->getUtilityCount(player);
    }
    return 0;
}

void GameContext::logEvent(const std::string& actionType, const std::string& detail) {
    if (logger) {
        int turn = 0;
        std::string username = "SYSTEM";
        if (turnManager != nullptr) {
            turn = turnManager->getCurrentTurn();
            Player* currentPlayer = turnManager->getCurrentPlayer();
            if (currentPlayer != nullptr) {
                username = currentPlayer->getUsername();
            }
        }
        logger->log(turn, username, actionType, detail);
    }
}
