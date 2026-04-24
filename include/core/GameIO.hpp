#pragma once

#include <exception>
#include <string>
#include <vector>

class TransactionLogger;
class ActionCard;
class Board;
class Player;
class PropertyTile;
class StreetTile;
class SkillCard;
class TurnManager;
class LogEntry;
enum class CardType;

class GameIO {
public:
    virtual ~GameIO() = default;

    virtual int promptInt(const std::string& prompt) = 0;
    virtual int promptIntInRange(const std::string& prompt, int minValue, int maxValue) = 0;
    virtual std::string promptText(const std::string& prompt);
    virtual int promptAuctionBid(const std::string& playerName, int highestBid, int balance);
    virtual bool confirmYN(const std::string& message) = 0;
    virtual void showMessage(const std::string& message) = 0;
    virtual void showError(
        const std::exception& exception,
        TransactionLogger* logger = nullptr,
        int turn = 0,
        const std::string& username = "SYSTEM"
    ) = 0;

    virtual void showDiceRoll(int die1, int die2);
    virtual void showPawnStep(const Player& player, int tileIndex);
    virtual bool confirmPropertyPurchase(const Player& player, const PropertyTile& property);
    virtual void showDiceLanding(
        int die1,
        int die2,
        int total,
        const std::string& playerName,
        const std::string& tileName,
        const std::string& tileCode
    );
    virtual void showPropertyNotice(const Player& player, const PropertyTile& property);
    virtual void showActionCard(CardType cardType, const ActionCard& card);
    virtual void showPaymentNotification(const std::string& title, const std::string& detail);
    virtual void showAuctionNotification(const std::string& title, const std::string& detail);
    virtual void showStreetPurchasePreview(
        const Player& player,
        const PropertyTile& tile,
        const StreetTile& street,
        int originalPrice,
        int finalPrice
    );
    virtual bool usesRichGuiPresentation() const;
    virtual int promptAuctionBid(const PropertyTile& property, const Player& bidder, int highestBid);
    virtual int promptTaxPaymentOption(
        const Player& player,
        const std::string& tileName,
        int flatAmount,
        int percentage,
        int wealth,
        int percentageAmount
    );
    virtual int promptTileSelection(const std::string& title, const std::vector<int>& validTileIndices);
    virtual int promptTileSelection(
        const std::string& title,
        const std::vector<int>& validTileIndices,
        bool allowCancel
    );
    virtual int promptSkillCardSelection(
        const std::string& title,
        const std::vector<SkillCard*>& cards,
        bool allowCancel
    );
    virtual void showHelp(const Player& player);
    virtual void renderBoard(const Board& board, const std::vector<Player>& players, const TurnManager& turnManager);
    virtual void showPropertyDeed(const PropertyTile* property);
    virtual void showPlayerProperties(const Player& player);
    virtual void showLogEntries(const std::vector<LogEntry>& entries);
};
