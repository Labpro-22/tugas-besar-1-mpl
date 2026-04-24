#include "views/GameSetupPage.hpp"

#include <algorithm>

#include <QButtonGroup>
#include <QFrame>
#include <QFont>
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

    outerLayout = new QVBoxLayout();
    outerLayout->setContentsMargins(0, 32, 0, 32);
    outerLayout->setAlignment(Qt::AlignCenter);
    rootLayout->addLayout(outerLayout, 1);

    setupCard = new QFrame(this);
    setupCard->setObjectName(QStringLiteral("setupCard"));
    setupCard->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    outerLayout->addWidget(setupCard, 0, Qt::AlignCenter);

    cardLayout = new QVBoxLayout(setupCard);
    cardLayout->setContentsMargins(52, 40, 52, 40);
    cardLayout->setSpacing(22);

    backButton = new QPushButton(QStringLiteral("<  BACK"), setupCard);
    backButton->setObjectName(QStringLiteral("backButton"));
    backButton->setCursor(Qt::PointingHandCursor);
    backButton->setFlat(true);
    backButton->setMaximumWidth(130);
    cardLayout->addWidget(backButton, 0, Qt::AlignLeft);

    titleLabel = new QLabel(QStringLiteral("Game Setup"), setupCard);
    titleLabel->setStyleSheet(QStringLiteral("color:#05070a;font:900 30pt 'Trebuchet MS';"));
    cardLayout->addWidget(titleLabel);

    subtitleLabel = new QLabel(QStringLiteral("Configure the player slots for the upcoming match."), setupCard);
    subtitleLabel->setStyleSheet(QStringLiteral("color:#343a43;font:500 15pt 'Trebuchet MS';"));
    subtitleLabel->setWordWrap(true);
    cardLayout->addWidget(subtitleLabel);

    countLabel = new QLabel(QStringLiteral("NUMBER OF PLAYERS"), setupCard);
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

    playerDetailsLabel = new QLabel(QStringLiteral("PLAYER DETAILS"), setupCard);
    playerDetailsLabel->setStyleSheet(QStringLiteral("color:#101318;font:800 11pt 'Trebuchet MS';letter-spacing:1px;"));
    cardLayout->addWidget(playerDetailsLabel);

    playerGrid = new QGridLayout();
    playerGrid->setHorizontalSpacing(18);
    playerGrid->setVerticalSpacing(18);
    playerGrid->setColumnStretch(0, 1);
    playerGrid->setColumnStretch(1, 1);
    cardLayout->addLayout(playerGrid);

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
        playerDots.append(dot);

        auto* playerLabelWidget = new QLabel(playerLabel(index), playerCard);
        playerLabelWidget->setStyleSheet(QStringLiteral("color:#222831;font:800 11pt 'Trebuchet MS';"));
        labelRow->addWidget(playerLabelWidget);
        playerNameLabels.append(playerLabelWidget);
        labelRow->addStretch(1);

        auto* input = new QLineEdit(playerCard);
        input->setPlaceholderText(QStringLiteral("Enter name..."));
        input->setMinimumHeight(54);
        input->setProperty("playerInput", true);
        playerCardLayout->addWidget(input);

        playerCards.append(playerCard);
        playerInputs.append(input);
        playerGrid->addWidget(playerCard, index / 2, index % 2);
    }

    auto* bottomRow = new QHBoxLayout();
    bottomRow->addStretch(1);

    startButton = new QPushButton(QStringLiteral("START GAME"), setupCard);
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
    const int pageWidth = qMax(width(), minimumWidth());
    const int pageHeight = qMax(height(), minimumHeight());
    const int setupWidth = std::clamp(static_cast<int>(pageWidth * 0.78), 520, 1120);
    const int sectionGap = std::clamp(pageHeight / 40, 14, 28);
    const int cardPaddingX = std::clamp(pageWidth / 28, 26, 68);
    const int cardPaddingY = std::clamp(pageHeight / 24, 24, 52);
    const int titleSize = std::clamp(pageWidth / 34, 18, 30);
    const int subtitleSize = std::clamp(pageWidth / 64, 10, 15);
    const int sectionTitleSize = std::clamp(pageWidth / 90, 9, 12);
    const int countButtonHeight = std::clamp(pageHeight / 11, 50, 74);
    const int inputHeight = std::clamp(pageHeight / 10, 48, 66);
    const int startHeight = std::clamp(pageHeight / 9, 58, 78);
    const int startWidth = std::clamp(static_cast<int>(setupWidth * 0.28), 220, 320);
    const int labelDotSize = std::clamp(pageWidth / 60, 14, 18);

    if (outerLayout != nullptr) {
        outerLayout->setContentsMargins(0, std::clamp(pageHeight / 18, 18, 42), 0, std::clamp(pageHeight / 18, 18, 42));
    }

    if (setupCard != nullptr) {
        setupCard->setMinimumWidth(std::clamp(static_cast<int>(pageWidth * 0.54), 480, 780));
        setupCard->setMaximumWidth(setupWidth);
    }

    if (cardLayout != nullptr) {
        cardLayout->setContentsMargins(cardPaddingX, cardPaddingY, cardPaddingX, cardPaddingY);
        cardLayout->setSpacing(sectionGap);
    }

    if (playerGrid != nullptr) {
        const int gridGap = std::clamp(pageWidth / 60, 12, 24);
        const int columnCount = setupWidth >= 760 ? 2 : 1;
        playerGrid->setHorizontalSpacing(gridGap);
        playerGrid->setVerticalSpacing(gridGap);
        for (int index = 0; index < playerCards.size(); ++index) {
            playerGrid->addWidget(playerCards[index], index / columnCount, index % columnCount);
        }
    }

    if (titleLabel != nullptr) {
        titleLabel->setStyleSheet(QStringLiteral("color:#05070a;font:900 %1pt 'Trebuchet MS';").arg(titleSize));
    }

    if (subtitleLabel != nullptr) {
        subtitleLabel->setStyleSheet(QStringLiteral("color:#343a43;font:500 %1pt 'Trebuchet MS';").arg(subtitleSize));
    }

    const QString sectionLabelStyle = QStringLiteral(
        "color:#101318;font:800 %1pt 'Trebuchet MS';letter-spacing:1px;"
    ).arg(sectionTitleSize);
    if (countLabel != nullptr) {
        countLabel->setStyleSheet(QStringLiteral("margin-top:%1px;%2").arg(std::clamp(sectionGap - 4, 10, 18)).arg(sectionLabelStyle));
    }
    if (playerDetailsLabel != nullptr) {
        playerDetailsLabel->setStyleSheet(sectionLabelStyle);
    }

    if (backButton != nullptr) {
        backButton->setMaximumWidth(std::clamp(pageWidth / 7, 110, 150));
        QFont font(QStringLiteral("Trebuchet MS"), std::clamp(sectionTitleSize, 9, 11), QFont::Bold);
        backButton->setFont(font);
    }

    const QList<QPushButton*> countButtons = {twoPlayersButton, threePlayersButton, fourPlayersButton};
    for (QPushButton* button : countButtons) {
        if (button == nullptr) {
            continue;
        }
        button->setMinimumHeight(countButtonHeight);
        QFont font(QStringLiteral("Trebuchet MS"), std::clamp(countButtonHeight / 5, 11, 16), QFont::Black);
        button->setFont(font);
    }

    for (QWidget* playerCard : playerCards) {
        auto* layout = qobject_cast<QVBoxLayout*>(playerCard != nullptr ? playerCard->layout() : nullptr);
        if (layout != nullptr) {
            layout->setSpacing(std::clamp(sectionGap / 2, 6, 12));
        }
    }

    for (int index = 0; index < playerDots.size(); ++index) {
        QLabel* dot = playerDots[index];
        if (dot == nullptr) {
            continue;
        }
        dot->setFixedSize(labelDotSize, labelDotSize);
        dot->setStyleSheet(QStringLiteral("background:%1;border-radius:%2px;")
            .arg(playerAccent(index).name())
            .arg(labelDotSize / 2));
    }

    for (QLabel* label : playerNameLabels) {
        if (label == nullptr) {
            continue;
        }
        label->setStyleSheet(QStringLiteral("color:#222831;font:800 %1pt 'Trebuchet MS';")
            .arg(std::clamp(sectionTitleSize, 10, 13)));
    }

    for (QLineEdit* input : playerInputs) {
        if (input == nullptr) {
            continue;
        }
        input->setMinimumHeight(inputHeight);
        QFont font(QStringLiteral("Trebuchet MS"), std::clamp(inputHeight / 5, 10, 14));
        input->setFont(font);
    }

    if (startButton != nullptr) {
        startButton->setMinimumSize(startWidth, startHeight);
        QFont font(QStringLiteral("Trebuchet MS"), std::clamp(startHeight / 5, 11, 15), QFont::Black);
        startButton->setFont(font);
    }
}
