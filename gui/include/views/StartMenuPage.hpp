#pragma once

#include <QWidget>

class QLabel;
class QPushButton;
class QToolButton;

class StartMenuPage : public QWidget
{
    Q_OBJECT

public:
    explicit StartMenuPage(QWidget* parent = nullptr);

signals:
    void newGameRequested();
    void loadGameRequested();
    void settingsRequested();
    void leaderboardRequested();

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    void updateResponsiveLayout();

    QPushButton* newGameButton = nullptr;
    QPushButton* loadGameButton = nullptr;
    QToolButton* settingsButton = nullptr;
    QToolButton* leaderboardButton = nullptr;
    QLabel* titleLabel = nullptr;
    QLabel* subtitleLabel = nullptr;
};
