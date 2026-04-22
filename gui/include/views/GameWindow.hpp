#pragma once

#include <QColor>
#include <QMap>
#include <QString>
#include <QVector>
#include <QWidget>

#include <vector>

#include "models/config/ConfigData.hpp"
#include "models/config/PropertyConfig.hpp"
#include "views/PropertyPortfolioWidget.hpp"

class BoardWidget;
class QLabel;
class PropertyCardWidget;
class PropertyPortfolioWidget;
class QButtonGroup;
class QDialog;
class QHBoxLayout;
class QFrame;
class QToolButton;
class QVBoxLayout;

struct PlayerOverview {
    QString name;
    QString pawnAssetName;
    QColor accentColor;
    int balance = 0;
    int tileIndex = 0;
    QVector<int> ownedPropertyIds;
    bool isCurrentTurn = false;
    bool isInJail = false;
};

struct DemoPropertyState {
    QString ownerUsername;
    bool mortgaged = false;
    int buildingLevel = 0;
};

struct DemoHistoryEntry {
    int turn = 1;
    QString username;
    QString actionType;
    QString detail;
};

class GameWindow : public QWidget
{
public:
    explicit GameWindow(QWidget *parent = nullptr);

private:
    void loadConfigData();
    void initializeGame();
    void refreshSidebar();
    void refreshPlayerHeader();
    void refreshPlayerSelector();
    void refreshPortfolio();
    void refreshStats();
    void refreshHistory();
    void refreshBoardPawns();
    void syncSelectedPlayer();
    void setSelectedPlayer(const QString& username);
    void setSelectedProperty(int propertyId, bool showPreview = false);
    void showPropertyCard(int propertyId);
    void handleRollRequested();
    void saveCurrentGame();
    void refreshScene(const QString& statusText = QString());

    PlayerOverview* playerOverviewByUsername(const QString& username);
    const PlayerOverview* playerOverviewByUsername(const QString& username) const;
    const PropertyConfig* propertyConfigForId(int propertyId) const;
    const DemoPropertyState* propertyStateForId(int propertyId) const;
    QVector<int> ownedPropertyIds(const QString& username) const;
    QVector<PortfolioPropertyView> buildPortfolioForPlayer(const QString& username) const;
    QString pawnAssetNameForPlayer(const QString& username) const;
    QColor accentColorForPlayer(const QString& username) const;
    int totalHouseCount(const QString& username) const;
    int totalHotelCount(const QString& username) const;

    BoardWidget* boardWidget = nullptr;
    QWidget* sidebarPanel = nullptr;
    QLabel* playerAvatarLabel = nullptr;
    QLabel* playerNameLabel = nullptr;
    QLabel* playerMoneyLabel = nullptr;
    QLabel* playerMetaLabel = nullptr;
    QHBoxLayout* playerSelectorLayout = nullptr;
    QButtonGroup* playerSelectorGroup = nullptr;
    PropertyPortfolioWidget* portfolioWidget = nullptr;
    QFrame* historyHeaderFrame = nullptr;
    QLabel* houseCountLabel = nullptr;
    QLabel* hotelCountLabel = nullptr;
    QToolButton* rollButton = nullptr;
    QToolButton* buildButton = nullptr;
    QToolButton* mortgageButton = nullptr;
    QToolButton* saveButton = nullptr;
    QVBoxLayout* historyEntriesLayout = nullptr;
    QDialog* propertyCardDialog = nullptr;
    PropertyCardWidget* propertyCardWidget = nullptr;

    ConfigData configData;
    bool hasConfigData = false;
    std::vector<PropertyConfig> properties;
    QMap<QString, QString> pawnAssetByPlayer;
    QMap<QString, QColor> accentColorByPlayer;
    QVector<PlayerOverview> playerOverviews;
    QMap<int, DemoPropertyState> propertyStateById;
    QVector<DemoHistoryEntry> historyEntries;
    QString activePlayerUsername;
    QString selectedPlayerUsername;
    QString lastStatusText;
    int selectedPropertyId = 0;
    int currentTurn = 1;
};
