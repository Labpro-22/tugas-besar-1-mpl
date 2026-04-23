#pragma once

#include <QString>
#include <QStringList>

#include <functional>
#include <string>
#include <vector>

#include "core/AuctionManager.hpp"
#include "core/BankruptcyHandler.hpp"
#include "core/Board.hpp"
#include "core/CommandProcessor.hpp"
#include "core/Dice.hpp"
#include "core/GameContext.hpp"
#include "core/QtGameIO.hpp"
#include "core/TurnManager.hpp"
#include "models/Player.hpp"
#include "models/cards/ActionCard.hpp"
#include "models/cards/SkillCard.hpp"
#include "models/config/ConfigData.hpp"
#include "models/state/GameState.hpp"
#include "utils/CardDeck.hpp"
#include "utils/TransactionLogger.hpp"

class QWidget;
class Command;
class PropertyTile;

class GuiGameSession
{
public:
    explicit GuiGameSession(QWidget* dialogParent = nullptr);
    ~GuiGameSession();

    bool loadConfig(QString* errorMessage = nullptr);
    void setMovementStepHandler(std::function<void(const Player&, int)> handler);
    void setPropertyPurchaseHandler(std::function<bool(const Player&, const PropertyTile&)> handler);
    void setTurnChangedHandler(std::function<void()> handler);
    bool startNewGame(const std::vector<std::string>& playerNames, QString* errorMessage = nullptr);
    bool loadGame(const std::string& filename, QString* errorMessage = nullptr);
    bool saveGame(const std::string& filename, QString* errorMessage = nullptr);

    bool rollDice(QString* errorMessage = nullptr);
    bool setDiceAndRoll(int die1, int die2, QString* errorMessage = nullptr);
    bool build(QString* errorMessage = nullptr);
    bool mortgage(QString* errorMessage = nullptr);
    bool redeem(QString* errorMessage = nullptr);
    bool useSkill(QString* errorMessage = nullptr);
    bool payJailFine(QString* errorMessage = nullptr);

    bool isReady() const;
    bool isGameEnded() const;
    const ConfigData& getConfigData() const;
    const std::vector<Player>& getPlayers() const;
    const TransactionLogger& getLogger() const;
    GameState snapshot() const;
    std::vector<Player*> determineWinner() const;
    Player* getCurrentPlayer();
    const Player* getCurrentPlayer() const;
    int getCurrentTurn() const;
    int getMaxTurn() const;
    QStringList takePendingMessages();
    QString currentPlayerUsername() const;

private:
    bool executeCommand(const Command& command, QString* errorMessage);
    bool ensureTurnPrepared(QString* errorMessage);
    bool beginCurrentTurn(QString* errorMessage);
    bool finishTurnAndAdvance(QString* errorMessage);
    bool checkGameEnd() const;
    void rebuildContext();
    void resetSessionState();
    void initializeBoardAndDecks();
    void randomizeTurnOrder();
    std::vector<Player*> buildPlayerPointers();
    std::vector<Player*> buildPlayerPointers() const;
    bool handleException(const std::exception& exception, QString* errorMessage);

    Board board;
    Dice dice;
    TurnManager turnManager;
    AuctionManager auctionManager;
    BankruptcyHandler bankruptcyHandler;
    TransactionLogger logger;
    QtGameIO io;

    CardDeck<ActionCard> chanceDeck;
    CardDeck<ActionCard> communityDeck;
    CardDeck<SkillCard> skillDeck;

    GameContext* context = nullptr;
    ConfigData configData;
    std::vector<Player> players;
    CommandProcessor commandProcessor;

    bool configLoaded = false;
    bool gameReady = false;
    bool currentTurnPrepared = false;
    bool gameEnded = false;
    std::function<void()> turnChangedHandler;
};
