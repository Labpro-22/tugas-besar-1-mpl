#pragma once

#include <QString>
#include <QStringList>

#include <functional>
#include <string>
#include <vector>

#include "core/GameEngine.hpp"
#include "core/QtGameIO.hpp"
#include "models/config/ConfigData.hpp"
#include "models/state/GameState.hpp"
#include "utils/TransactionLogger.hpp"

class Command;
class Player;
class PropertyTile;
class QWidget;

class GuiGameSession
{
public:
    explicit GuiGameSession(QWidget* dialogParent = nullptr);

    bool loadConfig(const QString& configDir, QString* errorMessage = nullptr);
    void setMovementStepHandler(std::function<void(const Player&, int)> handler);
    void setPropertyPurchaseHandler(std::function<bool(const Player&, const PropertyTile&)> handler);
    void setPropertyNoticeHandler(std::function<void(const Player&, const PropertyTile&)> handler);
    void setBoardTileSelectionHandler(std::function<int(const QString&, const QVector<int>&, bool)> handler);
    void setLiquidationPlanHandler(std::function<bool(
        const Player&,
        int,
        const std::vector<LiquidationCandidate>&,
        std::vector<LiquidationDecision>&
    )> handler);
    void setTurnChangedHandler(std::function<void()> handler);

    bool startNewGame(const QString& configDir, const std::vector<std::string>& playerNames, QString* errorMessage = nullptr);
    bool loadGame(const std::string& filename, QString* errorMessage = nullptr);
    bool saveGame(const std::string& filename, QString* errorMessage = nullptr);

    bool rollDice(QString* errorMessage = nullptr);
    bool setDiceAndRoll(int die1, int die2, QString* errorMessage = nullptr);
    bool build(QString* errorMessage = nullptr);
    bool mortgage(QString* errorMessage = nullptr);
    bool redeem(QString* errorMessage = nullptr);
    bool useSkill(QString* errorMessage = nullptr);
    bool payJailFine(QString* errorMessage = nullptr);
    bool surrender(QString* errorMessage = nullptr);

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
    bool handleException(const std::exception& exception, QString* errorMessage);

    TransactionLogger logger;
    QtGameIO io;
    GameEngine coreSession;
    ConfigData configData;
    bool configLoaded = false;
};
