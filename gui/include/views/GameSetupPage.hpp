#pragma once

#include <QWidget>

class QButtonGroup;
class QLabel;
class QLineEdit;
class QPushButton;

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

private:
    void applyPlayerCount(int count);

    QButtonGroup* countGroup = nullptr;
    QPushButton* twoPlayersButton = nullptr;
    QPushButton* threePlayersButton = nullptr;
    QPushButton* fourPlayersButton = nullptr;
    QList<QWidget*> playerCards;
    QList<QLineEdit*> playerInputs;
};
