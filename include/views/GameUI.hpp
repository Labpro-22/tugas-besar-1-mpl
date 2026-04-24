#pragma once

#include <string>
#include <vector>

#include "core/GameContext.hpp"
#include "core/GameIO.hpp"
#include "models/Player.hpp"
#include "models/state/Command.hpp"
#include "models/state/LogEntry.hpp"
#include "views/BoardRenderer.hpp"
#include "views/PropertyCardRenderer.hpp"

class TransactionLogger;

class GameUI : public GameIO {
private:
    BoardRenderer boardRenderer;
    PropertyCardRenderer propRenderer;

    Command readCommand();

public:
    int showMainMenu();
    Command promptLoadCommand();
    int promptPlayerCount();
    std::vector<std::string> promptPlayerNames(int n);

    bool confirmYN(const std::string& message) override;
    int promptInt(const std::string& prompt) override;
    int promptIntInRange(const std::string& prompt, int minValue, int maxValue) override;
    std::string promptText(const std::string& prompt) override;
    int promptAuctionBid(const std::string& playerName, int highestBid, int balance) override;
    int promptAuctionBid(const PropertyTile& property, const Player& bidder, int highestBid) override;
    int promptTaxPaymentOption(
        const Player& player,
        const std::string& tileName,
        int flatAmount,
        int percentage,
        int wealth,
        int percentageAmount
    ) override;
    int promptTileSelection(const std::string& title, const std::vector<int>& validTileIndices) override;
    int promptTileSelection(
        const std::string& title,
        const std::vector<int>& validTileIndices,
        bool allowCancel
    ) override;
    Command promptPlayerCommand(const std::string& username);
    void showMessage(const std::string& message) override;
    void showError(
        const std::exception& exception,
        TransactionLogger* logger = nullptr,
        int turn = 0,
        const std::string& username = "SYSTEM"
    ) override;
    void showUnknownError(
        TransactionLogger* logger = nullptr,
        int turn = 0,
        const std::string& username = "SYSTEM"
    );
    void showPropertyNotice(const Player& player, const PropertyTile& property) override;
    void showPaymentNotification(const std::string& title, const std::string& detail) override;
    void showAuctionNotification(const std::string& title, const std::string& detail) override;
    void showStreetPurchasePreview(
        const Player& player,
        const PropertyTile& tile,
        const StreetTile& street,
        int originalPrice,
        int finalPrice
    ) override;
    void showHelp(const Player& player) override;
    void showSection(const std::string& title);
    void showTurnSummary(const Player& player);
    void showDiceLanding(
        int die1,
        int die2,
        int total,
        const std::string& playerName,
        const std::string& tileName,
        const std::string& tileCode
    ) override;
    void showWinner(
        const std::vector<Player*>& winners,
        const std::vector<Player>& players,
        GameContext& context
    );

    void showLogEntries(const std::vector<LogEntry>& entries) override;
    void renderBoard(const Board& board, const std::vector<Player>& players, const TurnManager& turnManager) override;
    void showPropertyDeed(const PropertyTile* property) override;
    void showPlayerProperties(const Player& player) override;

    BoardRenderer& getBoardRenderer();
    PropertyCardRenderer& getPropertyCardRenderer();
};
