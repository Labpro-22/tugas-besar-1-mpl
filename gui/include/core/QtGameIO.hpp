#pragma once

#include <QStringList>
#include <QVector>

#include <functional>

#include "core/GameIO.hpp"

class ActionCard;
class QWidget;
class Player;
class PropertyTile;
class StreetTile;
class SkillCard;
enum class CardType;

class QtGameIO : public GameIO
{
public:
    explicit QtGameIO(QWidget* dialogParent = nullptr);

    void setDialogParent(QWidget* dialogParent);
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

    int promptInt(const std::string& prompt) override;
    int promptIntInRange(const std::string& prompt, int minValue, int maxValue) override;
    bool confirmYN(const std::string& message) override;
    void showDiceRoll(int die1, int die2) override;
    void showPawnStep(const Player& player, int tileIndex) override;
    bool confirmPropertyPurchase(const Player& player, const PropertyTile& property) override;
    void showPropertyNotice(const Player& player, const PropertyTile& property) override;
    void showActionCard(CardType cardType, const ActionCard& card) override;
    void showPaymentNotification(const std::string& title, const std::string& detail) override;
    void showAuctionNotification(const std::string& title, const std::string& detail) override;
    void showStreetPurchasePreview(
        const Player& player,
        const PropertyTile& tile,
        const StreetTile& street,
        int originalPrice,
        int finalPrice
    ) override;
    bool usesRichGuiPresentation() const override;
    int promptAuctionBid(const PropertyTile& property, const Player& bidder, int highestBid) override;
    int promptAuctionBid(
        const PropertyTile& property,
        const Player& bidder,
        int highestBid,
        const std::string& highestBidderName
    ) override;
    int promptTaxPaymentOption(
        const Player& player,
        const std::string& tileName,
        int flatAmount,
        int percentage,
        int wealth,
        int percentageAmount
    ) override;
    bool promptLiquidationPlan(
        const Player& player,
        int targetAmount,
        const std::vector<LiquidationCandidate>& candidates,
        std::vector<LiquidationDecision>& decisions
    ) override;
    int promptTileSelection(const std::string& title, const std::vector<int>& validTileIndices) override;
    int promptTileSelection(
        const std::string& title,
        const std::vector<int>& validTileIndices,
        bool allowCancel
    ) override;
    int promptSkillCardSelection(
        const std::string& title,
        const std::vector<SkillCard*>& cards,
        bool allowCancel
    ) override;
    void showMessage(const std::string& message) override;
    void showError(
        const std::exception& exception,
        TransactionLogger* logger = nullptr,
        int turn = 0,
        const std::string& username = "SYSTEM"
    ) override;

    QStringList takePendingMessages();
    QString latestMessage() const;
    void clearMessages();

private:
    QWidget* dialogParent = nullptr;
    QStringList pendingMessages;
    std::function<void(const Player&, int)> movementStepHandler;
    std::function<bool(const Player&, const PropertyTile&)> propertyPurchaseHandler;
    std::function<void(const Player&, const PropertyTile&)> propertyNoticeHandler;
    std::function<int(const QString&, const QVector<int>&, bool)> boardTileSelectionHandler;
    std::function<bool(const Player&, int, const std::vector<LiquidationCandidate>&, std::vector<LiquidationDecision>&)> liquidationPlanHandler;
};
