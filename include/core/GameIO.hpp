#pragma once

#include <exception>
#include <string>
#include <vector>

class TransactionLogger;
class ActionCard;
class Player;
class PropertyTile;
class SkillCard;
enum class CardType;

class GameIO {
public:
    virtual ~GameIO() = default;

    virtual int promptInt(const std::string& prompt) = 0;
    virtual int promptIntInRange(const std::string& prompt, int minValue, int maxValue) = 0;
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
    virtual void showPropertyNotice(const Player& player, const PropertyTile& property);
    virtual void showActionCard(CardType cardType, const ActionCard& card);
    virtual void showPaymentNotification(const std::string& title, const std::string& detail);
    virtual void showAuctionNotification(const std::string& title, const std::string& detail);
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
};
