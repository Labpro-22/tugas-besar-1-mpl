#pragma once

#include <QStringList>

#include <functional>

#include "core/GameIO.hpp"

class ActionCard;
class QWidget;
class Player;
class PropertyTile;
class SkillCard;
enum class CardType;

class QtGameIO : public GameIO
{
public:
    explicit QtGameIO(QWidget* dialogParent = nullptr);

    void setDialogParent(QWidget* dialogParent);
    void setMovementStepHandler(std::function<void(const Player&, int)> handler);
    void setPropertyPurchaseHandler(std::function<bool(const Player&, const PropertyTile&)> handler);

    int promptInt(const std::string& prompt) override;
    int promptIntInRange(const std::string& prompt, int minValue, int maxValue) override;
    bool confirmYN(const std::string& message) override;
    void showDiceRoll(int die1, int die2) override;
    void showPawnStep(const Player& player, int tileIndex) override;
    bool confirmPropertyPurchase(const Player& player, const PropertyTile& property) override;
    void showActionCard(CardType cardType, const ActionCard& card) override;
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
};
