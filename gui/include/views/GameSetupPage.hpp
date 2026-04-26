#pragma once

#include <QWidget>

class QButtonGroup;
class QLabel;
class QLineEdit;
class QPushButton;
class QGridLayout;
class QScrollArea;
class QVBoxLayout;

class GameSetupPage : public QWidget
{
    Q_OBJECT

public:
    explicit GameSetupPage(QWidget* parent = nullptr);

    int playerCount() const;
    QStringList playerNames() const;
    void resetForm();

signals:
    void backRequested();
    void startRequested();

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    void applyPlayerCount(int count);
    void updateResponsiveLayout();

    QButtonGroup* countGroup = nullptr;
    QPushButton* twoPlayersButton = nullptr;
    QPushButton* threePlayersButton = nullptr;
    QPushButton* fourPlayersButton = nullptr;
    QPushButton* backButton = nullptr;
    QPushButton* startButton = nullptr;
    QLabel* titleLabel = nullptr;
    QLabel* subtitleLabel = nullptr;
    QLabel* countLabel = nullptr;
    QLabel* playerDetailsLabel = nullptr;
    QList<QLabel*> playerDots;
    QList<QLabel*> playerNameLabels;
    QList<QWidget*> playerCards;
    QList<QLineEdit*> playerInputs;
    QWidget* setupCard = nullptr;
    QScrollArea* setupScrollArea = nullptr;
    QWidget* scrollContent = nullptr;
    QVBoxLayout* outerLayout = nullptr;
    QVBoxLayout* cardLayout = nullptr;
    QGridLayout* playerGrid = nullptr;
};
