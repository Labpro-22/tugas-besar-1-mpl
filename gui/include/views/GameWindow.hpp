#pragma once

#include <QColor>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QWidget>

#include <functional>
#include <vector>

#include "core/GuiGameSession.hpp"
#include "models/config/PropertyConfig.hpp"
#include "views/PropertyPortfolioWidget.hpp"

class BoardWidget;
class GameSetupPage;
class QLabel;
class PropertyCardWidget;
class PropertyPortfolioWidget;
class QDialog;
class QFrame;
class QStackedWidget;
class QToolButton;
class QVBoxLayout;
class StartMenuPage;
class Player;
class PropertyTile;

struct PlayerOverview {
    QString name;
    QString pawnAssetName;
    QColor accentColor;
    int balance = 0;
    int tileIndex = 0;
    int handCount = 0;
    int propertyCount = 0;
    bool isCurrentTurn = false;
    bool isInJail = false;
    bool hasRolledThisTurn = false;
    bool hasUsedSkillThisTurn = false;
    bool hasTakenActionThisTurn = false;
};

struct PropertyViewState {
    QString ownerUsername;
    bool mortgaged = false;
    int buildingLevel = 0;
};

struct HistoryEntryView {
    int turn = 1;
    QString username;
    QString actionType;
    QString detail;
};

class GameWindow : public QWidget
{
public:
    explicit GameWindow(QWidget* parent = nullptr);

private:
    void buildRootPages();
    QWidget* buildGamePage();
    void configureConnections();
    void configureSession();

    void startNewGame();
    void loadGameFromPicker();
    void saveCurrentGame();
    void handleManualRollRequested();
    void executeSessionAction(const QString& successFallback, const std::function<bool(QString*)>& action);
    void showGameFinishedDialogIfNeeded();

    void refreshViewModels();
    void refreshScene(const QString& statusText = QString());
    void refreshSidebar();
    void refreshPlayerHeader();
    void refreshPlayerSelector();
    void refreshPortfolio();
    void refreshStats();
    void refreshHistory();
    void refreshBoardPawns();
    void refreshActionAvailability();
    void syncSelectedPlayer();
    void syncSelectedProperty();
    void setSelectedPlayer(const QString& username);
    void setSelectedProperty(int propertyId, bool showPreview = false);
    void showPropertyCard(int propertyId);
    void showPropertyNotice(const Player& player, const PropertyTile& property);
    int promptBoardTileSelection(const QString& title, const QVector<int>& validTileIndices, bool allowCancel);
    bool promptPropertyPurchase(const Player& player, const PropertyTile& property);
    void applyPendingMessages(const QString& fallback = QString());

    const PlayerOverview* playerOverviewByUsername(const QString& username) const;
    const PropertyConfig* propertyConfigForId(int propertyId) const;
    const PropertyViewState* propertyStateForId(int propertyId) const;
    QVector<PortfolioPropertyView> buildPortfolioForPlayer(const QString& username) const;
    QString pawnAssetNameForPlayer(const QString& username) const;
    QColor accentColorForPlayer(const QString& username) const;
    int totalHouseCount(const QString& username) const;
    int totalHotelCount(const QString& username) const;
    QString currentTurnStatusText() const;
    QString saveDirectoryPath() const;

    GuiGameSession session;

    QStackedWidget* pageStack = nullptr;
    StartMenuPage* startMenuPage = nullptr;
    GameSetupPage* setupPage = nullptr;
    QWidget* gamePage = nullptr;

    BoardWidget* boardWidget = nullptr;
    QWidget* sidebarPanel = nullptr;
    QLabel* playerAvatarLabel = nullptr;
    QLabel* playerNameLabel = nullptr;
    QLabel* playerMoneyLabel = nullptr;
    QToolButton* playerSwitchButton = nullptr;
    PropertyPortfolioWidget* portfolioWidget = nullptr;
    QFrame* historyHeaderFrame = nullptr;
    QLabel* houseCountLabel = nullptr;
    QLabel* hotelCountLabel = nullptr;
    QToolButton* rollButton = nullptr;
    QToolButton* setDiceButton = nullptr;
    QToolButton* useSkillButton = nullptr;
    QToolButton* payFineButton = nullptr;
    QToolButton* buildButton = nullptr;
    QToolButton* mortgageButton = nullptr;
    QToolButton* redeemButton = nullptr;
    QToolButton* saveButton = nullptr;
    QVBoxLayout* historyEntriesLayout = nullptr;
    QDialog* propertyCardDialog = nullptr;
    PropertyCardWidget* propertyCardWidget = nullptr;

    std::vector<PropertyConfig> properties;
    QMap<QString, QColor> accentColorByPlayer;
    QVector<PlayerOverview> playerOverviews;
    QMap<int, PropertyViewState> propertyStateById;
    QVector<HistoryEntryView> historyEntries;
    QString activePlayerUsername;
    QString selectedPlayerUsername;
    QString lastStatusText;
    int selectedPropertyId = 0;
    bool finishDialogShown = false;
};
