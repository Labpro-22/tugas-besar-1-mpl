#include "core/GuiGameSession.hpp"

#include <exception>
#include <utility>

#include <QString>

#include "models/state/Command.hpp"
#include "utils/ConfigLoader.hpp"
#include "utils/SaveManager.hpp"
#include "utils/UiCommon.hpp"

GuiGameSession::GuiGameSession(QWidget* dialogParent)
    : io(dialogParent),
      coreSession(io, &logger)
{
}

bool GuiGameSession::loadConfig(const QString& configDir, QString* errorMessage)
{
    if (configDir.isEmpty()) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("Folder config belum dipilih.");
        }
        return false;
    }

    try {
        ConfigLoader loader(configDir.toStdString());
        configData = loader.loadAll();
        configLoaded = true;
        return true;
    } catch (const std::exception& exception) {
        if (errorMessage != nullptr) {
            *errorMessage = QString::fromStdString(exception.what());
        }
        return false;
    }
}

void GuiGameSession::setMovementStepHandler(std::function<void(const Player&, int)> handler)
{
    io.setMovementStepHandler(std::move(handler));
}

void GuiGameSession::setPropertyPurchaseHandler(std::function<bool(const Player&, const PropertyTile&)> handler)
{
    io.setPropertyPurchaseHandler(std::move(handler));
}

void GuiGameSession::setPropertyNoticeHandler(std::function<void(const Player&, const PropertyTile&)> handler)
{
    io.setPropertyNoticeHandler(std::move(handler));
}

void GuiGameSession::setBoardTileSelectionHandler(std::function<int(const QString&, const QVector<int>&, bool)> handler)
{
    io.setBoardTileSelectionHandler(std::move(handler));
}

void GuiGameSession::setLiquidationPlanHandler(std::function<bool(
    const Player&,
    int,
    const std::vector<LiquidationCandidate>&,
    std::vector<LiquidationDecision>&
)> handler)
{
    io.setLiquidationPlanHandler(std::move(handler));
}

void GuiGameSession::setTurnChangedHandler(std::function<void()> handler)
{
    coreSession.setTurnChangedHandler(std::move(handler));
}

bool GuiGameSession::startNewGame(const QString& configDir, const std::vector<std::string>& playerNames, QString* errorMessage)
{
    if (!loadConfig(configDir, errorMessage)) {
        return false;
    }

    try {
        io.clearMessages();
        coreSession.startNewGame(configData, playerNames);
        coreSession.prepareCurrentTurn();
        return true;
    } catch (const std::exception& exception) {
        return handleException(exception, errorMessage);
    }
}

bool GuiGameSession::loadGame(const std::string& filename, QString* errorMessage)
{
    try {
        io.clearMessages();
        SaveManager saveManager;
        GameState gameState = saveManager.loadGame(filename);
        QString configDir = QString::fromStdString(gameState.getConfigPath());
        if (configDir.isEmpty()) {
            configDir = MonopolyUi::findConfigDirectory();
        }
        if (!loadConfig(configDir, errorMessage)) {
            return false;
        }
        coreSession.loadGame(configData, gameState);
        return true;
    } catch (const std::exception& exception) {
        return handleException(exception, errorMessage);
    }
}

bool GuiGameSession::saveGame(const std::string& filename, QString* errorMessage)
{
    return executeCommand(Command("SIMPAN", {filename}), errorMessage);
}

bool GuiGameSession::rollDice(QString* errorMessage)
{
    return executeCommand(Command("LEMPAR_DADU", {}), errorMessage);
}

bool GuiGameSession::setDiceAndRoll(int die1, int die2, QString* errorMessage)
{
    return executeCommand(
        Command("ATUR_DADU", {std::to_string(die1), std::to_string(die2)}),
        errorMessage
    );
}

bool GuiGameSession::build(QString* errorMessage)
{
    return executeCommand(Command("BANGUN", {}), errorMessage);
}

bool GuiGameSession::mortgage(QString* errorMessage)
{
    return executeCommand(Command("GADAI", {}), errorMessage);
}

bool GuiGameSession::redeem(QString* errorMessage)
{
    return executeCommand(Command("UNMORTGAGE", {}), errorMessage);
}

bool GuiGameSession::useSkill(QString* errorMessage)
{
    return executeCommand(Command("GUNAKAN_KEMAMPUAN", {}), errorMessage);
}

bool GuiGameSession::payJailFine(QString* errorMessage)
{
    return executeCommand(Command("BAYAR_DENDA", {}), errorMessage);
}

bool GuiGameSession::surrender(QString* errorMessage)
{
    return executeCommand(Command("SURREND", {}), errorMessage);
}

bool GuiGameSession::isReady() const
{
    return coreSession.isReady();
}

bool GuiGameSession::isGameEnded() const
{
    return coreSession.isGameEnded();
}

const ConfigData& GuiGameSession::getConfigData() const
{
    if (configLoaded) {
        return configData;
    }

    return coreSession.getConfigData();
}

const std::vector<Player>& GuiGameSession::getPlayers() const
{
    return coreSession.getPlayers();
}

const TransactionLogger& GuiGameSession::getLogger() const
{
    return logger;
}

GameState GuiGameSession::snapshot() const
{
    return coreSession.snapshot();
}

std::vector<Player*> GuiGameSession::determineWinner() const
{
    return coreSession.determineWinner();
}

Player* GuiGameSession::getCurrentPlayer()
{
    return coreSession.getCurrentPlayer();
}

const Player* GuiGameSession::getCurrentPlayer() const
{
    return coreSession.getCurrentPlayer();
}

int GuiGameSession::getCurrentTurn() const
{
    return coreSession.getCurrentTurn();
}

int GuiGameSession::getMaxTurn() const
{
    return coreSession.getMaxTurn();
}

QStringList GuiGameSession::takePendingMessages()
{
    return io.takePendingMessages();
}

QString GuiGameSession::currentPlayerUsername() const
{
    const Player* currentPlayer = getCurrentPlayer();
    if (currentPlayer == nullptr) {
        return {};
    }

    return QString::fromStdString(currentPlayer->getUsername());
}

bool GuiGameSession::executeCommand(const Command& command, QString* errorMessage)
{
    try {
        const bool result = coreSession.executeCommand(command);
        coreSession.prepareCurrentTurn();
        return result;
    } catch (const std::exception& exception) {
        return handleException(exception, errorMessage);
    }
}

bool GuiGameSession::handleException(const std::exception& exception, QString* errorMessage)
{
    if (errorMessage != nullptr) {
        *errorMessage = QString::fromStdString(exception.what());
    }

    const Player* currentPlayer = getCurrentPlayer();
    io.showError(
        exception,
        &logger,
        getCurrentTurn(),
        currentPlayer == nullptr ? "SYSTEM" : currentPlayer->getUsername()
    );
    return false;
}
