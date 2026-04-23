#include "views/GameSetupPage.hpp"

#include <algorithm>

#include <QButtonGroup>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPaintEvent>
#include <QPainter>
#include <QPushButton>
#include <QResizeEvent>
#include <QVBoxLayout>

namespace {

QColor playerAccent(int index)
{
    switch (index) {
    case 0:
        return QColor(28, 97, 214);
    case 1:
        return QColor(216, 40, 40);
    case 2:
        return QColor(26, 30, 35);
    default:
        return QColor(126, 130, 138);
    }
}

QString playerLabel(int index)
{
    return QStringLiteral("Player %1").arg(index + 1);
}

}  // namespace

GameSetupPage::GameSetupPage(QWidget* parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("gameSetupPage"));

    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    auto* outerLayout = new QVBoxLayout();
    outerLayout->setContentsMargins(0, 32, 0, 32);
    outerLayout->setAlignment(Qt::AlignCenter);
    rootLayout->addLayout(outerLayout, 1);

    setupCard = new QFrame(this);
    setupCard->setObjectName(QStringLiteral("setupCard"));
    setupCard->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    outerLayout->addWidget(setupCard, 0, Qt::AlignCenter);

    auto* cardLayout = new QVBoxLayout(setupCard);
    cardLayout->setContentsMargins(52, 40, 52, 40);
    cardLayout->setSpacing(22);

    auto* backButton = new QPushButton(QStringLiteral("<  BACK"), setupCard);
    backButton->setObjectName(QStringLiteral("backButton"));
    backButton->setCursor(Qt::PointingHandCursor);
    backButton->setFlat(true);
    backButton->setMaximumWidth(130);
    cardLayout->addWidget(backButton, 0, Qt::AlignLeft);

    auto* titleLabel = new QLabel(QStringLiteral("Game Setup"), setupCard);
    titleLabel->setStyleSheet(QStringLiteral("color:#05070a;font:900 30pt 'Trebuchet MS';"));
    cardLayout->addWidget(titleLabel);

    auto* subtitleLabel = new QLabel(QStringLiteral("Configure the player slots for the upcoming match."), setupCard);
    subtitleLabel->setStyleSheet(QStringLiteral("color:#343a43;font:500 15pt 'Trebuchet MS';"));
    cardLayout->addWidget(subtitleLabel);

    auto* countLabel = new QLabel(QStringLiteral("NUMBER OF PLAYERS"), setupCard);
    countLabel->setStyleSheet(QStringLiteral("margin-top:18px;color:#101318;font:800 11pt 'Trebuchet MS';letter-spacing:1px;"));
    cardLayout->addWidget(countLabel);

    auto* countWrap = new QFrame(setupCard);
    countWrap->setObjectName(QStringLiteral("countWrap"));
    auto* countLayout = new QHBoxLayout(countWrap);
    countLayout->setContentsMargins(0, 0, 0, 0);
    countLayout->setSpacing(0);
    cardLayout->addWidget(countWrap);

    countGroup = new QButtonGroup(this);
    countGroup->setExclusive(true);

    twoPlayersButton = new QPushButton(QStringLiteral("2"), countWrap);
    threePlayersButton = new QPushButton(QStringLiteral("3"), countWrap);
    fourPlayersButton = new QPushButton(QStringLiteral("4"), countWrap);

    const QList<QPushButton*> countButtons = {twoPlayersButton, threePlayersButton, fourPlayersButton};
    for (int index = 0; index < countButtons.size(); ++index) {
        QPushButton* button = countButtons[index];
        button->setCheckable(true);
        button->setCursor(Qt::PointingHandCursor);
        button->setMinimumHeight(56);
        button->setProperty("segmented", true);
        countGroup->addButton(button, index + 2);
        countLayout->addWidget(button);
    }
    twoPlayersButton->setChecked(true);

    auto* divider = new QFrame(setupCard);
    divider->setObjectName(QStringLiteral("sectionDivider"));
    divider->setFixedHeight(1);
    cardLayout->addWidget(divider);

    auto* playerDetailsLabel = new QLabel(QStringLiteral("PLAYER DETAILS"), setupCard);
    playerDetailsLabel->setStyleSheet(QStringLiteral("color:#101318;font:800 11pt 'Trebuchet MS';letter-spacing:1px;"));
    cardLayout->addWidget(playerDetailsLabel);

    auto* grid = new QGridLayout();
    grid->setHorizontalSpacing(18);
    grid->setVerticalSpacing(18);
    cardLayout->addLayout(grid);

    for (int index = 0; index < 4; ++index) {
        auto* playerCard = new QFrame(setupCard);
        playerCard->setProperty("playerEntryCard", true);

        auto* playerCardLayout = new QVBoxLayout(playerCard);
        playerCardLayout->setContentsMargins(0, 0, 0, 0);
        playerCardLayout->setSpacing(8);

        auto* labelRow = new QHBoxLayout();
        labelRow->setContentsMargins(0, 0, 0, 0);
        labelRow->setSpacing(8);
        playerCardLayout->addLayout(labelRow);

        auto* dot = new QLabel(playerCard);
        dot->setFixedSize(16, 16);
        dot->setStyleSheet(QStringLiteral("background:%1;border-radius:8px;").arg(playerAccent(index).name()));
        labelRow->addWidget(dot, 0, Qt::AlignVCenter);

        auto* playerLabelWidget = new QLabel(playerLabel(index), playerCard);
        playerLabelWidget->setStyleSheet(QStringLiteral("color:#222831;font:800 11pt 'Trebuchet MS';"));
        labelRow->addWidget(playerLabelWidget);
        labelRow->addStretch(1);

        auto* input = new QLineEdit(playerCard);
        input->setPlaceholderText(QStringLiteral("Enter name..."));
        input->setMinimumHeight(54);
        input->setProperty("playerInput", true);
        playerCardLayout->addWidget(input);

        playerCards.append(playerCard);
        playerInputs.append(input);
        grid->addWidget(playerCard, index / 2, index % 2);
    }

    auto* bottomRow = new QHBoxLayout();
    bottomRow->addStretch(1);

    auto* startButton = new QPushButton(QStringLiteral("START GAME"), setupCard);
    startButton->setObjectName(QStringLiteral("startButton"));
    startButton->setCursor(Qt::PointingHandCursor);
    startButton->setMinimumSize(220, 68);
    bottomRow->addWidget(startButton);
    cardLayout->addLayout(bottomRow);

    setStyleSheet(
        "#gameSetupPage { background: transparent; }"
        "#setupCard {"
        "  background: rgba(255,255,255,0.92);"
        "  border: 1px solid rgba(217, 224, 233, 0.95);"
        "  border-radius: 22px;"
        "}"
        "#countWrap {"
        "  background: rgba(231, 234, 238, 0.95);"
        "  border-radius: 18px;"
        "  padding: 4px;"
        "}"
        "QPushButton[segmented='true'] {"
        "  border: none;"
        "  border-radius: 14px;"
        "  background: transparent;"
        "  color: #24292f;"
        "  font: 800 12pt 'Trebuchet MS';"
        "}"
        "QPushButton[segmented='true']:checked {"
        "  background: white;"
        "  border: 1px solid rgba(208, 215, 223, 0.95);"
        "}"
        "#sectionDivider { background: rgba(231, 234, 238, 1.0); margin: 18px 0; }"
        "QFrame[playerEntryCard='true'] { background: transparent; }"
        "QLineEdit[playerInput='true'] {"
        "  background: #f2f4f6;"
        "  border: 1px solid #d6dbe1;"
        "  border-radius: 10px;"
        "  padding: 0 16px;"
        "  color: #171b21;"
        "  font: 11pt 'Trebuchet MS';"
        "}"
        "#startButton {"
        "  background: #1159c7;"
        "  border: 1px solid #0d49a4;"
        "  border-radius: 12px;"
        "  color: white;"
        "  font: 900 12pt 'Trebuchet MS';"
        "  letter-spacing: 1px;"
        "}"
        "#backButton {"
        "  color: #252d37;"
        "  font: 800 11pt 'Trebuchet MS';"
        "}"
        "#backButton:hover { color:#1159c7; }"
    );

    connect(backButton, &QPushButton::clicked, this, &GameSetupPage::backRequested);
    connect(startButton, &QPushButton::clicked, this, &GameSetupPage::startRequested);
    connect(countGroup, &QButtonGroup::idClicked, this, [this](int id) {
        applyPlayerCount(id);
    });

    applyPlayerCount(2);
    updateResponsiveLayout();
}

int GameSetupPage::playerCount() const
{
    return countGroup->checkedId() > 0 ? countGroup->checkedId() : 2;
}

QStringList GameSetupPage::playerNames() const
{
    QStringList names;
    const int count = playerCount();
    for (int index = 0; index < count; ++index) {
        names.append(playerInputs[index]->text().trimmed());
    }
    return names;
}

void GameSetupPage::resetForm()
{
    twoPlayersButton->setChecked(true);
    applyPlayerCount(2);
    for (QLineEdit* input : playerInputs) {
        input->clear();
    }
}

void GameSetupPage::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QLinearGradient gradient(rect().topLeft(), rect().bottomLeft());
    gradient.setColorAt(0.0, QColor(246, 248, 252));
    gradient.setColorAt(1.0, QColor(238, 242, 247));
    painter.fillRect(rect(), gradient);

    QRadialGradient glow(QPointF(width() * 0.74, height() * 0.12), width() * 0.26);
    glow.setColorAt(0.0, QColor(197, 218, 255, 110));
    glow.setColorAt(1.0, QColor(197, 218, 255, 0));
    painter.setPen(Qt::NoPen);
    painter.setBrush(glow);
    painter.drawEllipse(QPointF(width() * 0.74, height() * 0.12), width() * 0.28, height() * 0.20);
}

void GameSetupPage::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    updateResponsiveLayout();
}

void GameSetupPage::applyPlayerCount(int count)
{
    for (int index = 0; index < playerCards.size(); ++index) {
        playerCards[index]->setVisible(index < count);
    }
}

void GameSetupPage::updateResponsiveLayout()
{
    if (setupCard != nullptr) {
        setupCard->setMaximumWidth(std::clamp(width() - 80, 520, 780));
    }
}
