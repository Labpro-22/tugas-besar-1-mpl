#include "views/GameWindow.hpp"

#include <algorithm>
#include <exception>
#include <vector>

#include <QAbstractButton>
#include <QButtonGroup>
#include <QDialog>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLayoutItem>
#include <QMessageBox>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QPushButton>
#include <QRandomGenerator>
#include <QScrollArea>
#include <QToolButton>
#include <QVBoxLayout>

#include "utils/ConfigLoader.hpp"
#include "utils/UiCommon.hpp"
#include "views/BoardWidget.hpp"
#include "views/PropertyCardWidget.hpp"
#include "views/PropertyPortfolioWidget.hpp"

namespace {

QString specialTileLabel(int tileIndex)
{
    switch (tileIndex) {
    case 0:
        return QStringLiteral("GO");
    case 2:
    case 17:
        return QStringLiteral("Dana Umum");
    case 4:
        return QStringLiteral("PPH");
    case 7:
    case 33:
        return QStringLiteral("Festival");
    case 10:
        return QStringLiteral("Penjara");
    case 20:
        return QStringLiteral("Bebas Parkir");
    case 22:
    case 36:
        return QStringLiteral("Kesempatan");
    case 30:
        return QStringLiteral("Go To Jail");
    case 38:
        return QStringLiteral("PPNBM");
    default:
        return QStringLiteral("Petak %1").arg(tileIndex + 1);
    }
}

QPixmap loadPawnPixmap(const QString& assetName, const QSize& desiredSize)
{
    QPixmap pixmap;
    const QString pawnDir = MonopolyUi::findPawnDirectory();
    if (!pawnDir.isEmpty()) {
        pixmap.load(pawnDir + '/' + assetName);
    }

    if (pixmap.isNull()) {
        return {};
    }

    return pixmap.scaled(desiredSize, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
}

QString shortPlayerLabel(const QString& username)
{
    const QStringList parts = username.split(' ', Qt::SkipEmptyParts);
    if (parts.isEmpty()) {
        return username.left(2).toUpper();
    }

    const QString last = parts.back();
    if (last.size() <= 4) {
        return last.toUpper();
    }

    return username.left(2).toUpper();
}

QString historyAccentColor(const DemoHistoryEntry& entry)
{
    const QString action = entry.actionType.toUpper();
    if (action == QStringLiteral("SEWA")) {
        return QStringLiteral("#ff5f5f");
    }
    if (action == QStringLiteral("BELI")) {
        return QStringLiteral("#ff9832");
    }
    if (action == QStringLiteral("SYSTEM")) {
        return QStringLiteral("#2e8ef7");
    }
    if (action == QStringLiteral("LEWAT")) {
        return QStringLiteral("#95a7b3");
    }
    return QStringLiteral("#48a45b");
}

enum class GlyphKind {
    Dice,
    Build,
    Mortgage,
    Save,
    House,
    Hotel
};

QPixmap makeGlyphPixmap(GlyphKind kind, const QSize& size, const QColor& color = QColor(62, 74, 95))
{
    QPixmap pixmap(size);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(color, qMax(1.6, size.width() * 0.08), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(Qt::NoBrush);

    const QRectF rect(1.5, 1.5, size.width() - 3.0, size.height() - 3.0);

    if (kind == GlyphKind::Dice) {
        painter.setBrush(color);
        painter.drawRoundedRect(rect.adjusted(size.width() * 0.18, size.height() * 0.14, -size.width() * 0.18, -size.height() * 0.14), 4, 4);
        painter.setBrush(Qt::white);
        const QRectF dice = rect.adjusted(size.width() * 0.22, size.height() * 0.18, -size.width() * 0.22, -size.height() * 0.18);
        const qreal r = qMax<qreal>(1.2, size.width() * 0.05);
        const QList<QPointF> pips = {
            QPointF(dice.left() + dice.width() * 0.28, dice.top() + dice.height() * 0.28),
            QPointF(dice.right() - dice.width() * 0.28, dice.top() + dice.height() * 0.28),
            QPointF(dice.center().x(), dice.center().y()),
            QPointF(dice.left() + dice.width() * 0.28, dice.bottom() - dice.height() * 0.28),
            QPointF(dice.right() - dice.width() * 0.28, dice.bottom() - dice.height() * 0.28)
        };
        for (const QPointF& pip : pips) {
            painter.drawEllipse(pip, r, r);
        }
        return pixmap;
    }

    if (kind == GlyphKind::Build) {
        painter.drawLine(QPointF(rect.left() + rect.width() * 0.25, rect.bottom() - rect.height() * 0.22),
                         QPointF(rect.right() - rect.width() * 0.18, rect.top() + rect.height() * 0.18));
        painter.drawLine(QPointF(rect.left() + rect.width() * 0.18, rect.top() + rect.height() * 0.24),
                         QPointF(rect.right() - rect.width() * 0.26, rect.bottom() - rect.height() * 0.18));
        painter.drawArc(QRectF(rect.left() + rect.width() * 0.52, rect.top() + rect.height() * 0.10, rect.width() * 0.20, rect.height() * 0.20), 40 * 16, 220 * 16);
        painter.drawLine(QPointF(rect.left() + rect.width() * 0.18, rect.top() + rect.height() * 0.24),
                         QPointF(rect.left() + rect.width() * 0.10, rect.top() + rect.height() * 0.16));
        return pixmap;
    }

    if (kind == GlyphKind::Mortgage) {
        QPainterPath house;
        house.moveTo(rect.left() + rect.width() * 0.18, rect.top() + rect.height() * 0.48);
        house.lineTo(rect.center().x(), rect.top() + rect.height() * 0.20);
        house.lineTo(rect.right() - rect.width() * 0.18, rect.top() + rect.height() * 0.48);
        house.lineTo(rect.right() - rect.width() * 0.18, rect.bottom() - rect.height() * 0.14);
        house.lineTo(rect.left() + rect.width() * 0.18, rect.bottom() - rect.height() * 0.14);
        house.closeSubpath();
        painter.drawPath(house);
        painter.drawLine(QPointF(rect.left() + rect.width() * 0.08, rect.bottom() - rect.height() * 0.20),
                         QPointF(rect.left() + rect.width() * 0.08, rect.top() + rect.height() * 0.40));
        painter.drawLine(QPointF(rect.left() + rect.width() * 0.08, rect.bottom() - rect.height() * 0.20),
                         QPointF(rect.left() + rect.width() * 0.22, rect.bottom() - rect.height() * 0.20));
        return pixmap;
    }

    if (kind == GlyphKind::Save) {
        painter.setBrush(color);
        painter.drawRoundedRect(rect.adjusted(size.width() * 0.20, size.height() * 0.10, -size.width() * 0.20, -size.height() * 0.12), 3, 3);
        painter.setBrush(Qt::white);
        painter.drawRect(QRectF(rect.left() + rect.width() * 0.30, rect.top() + rect.height() * 0.16, rect.width() * 0.28, rect.height() * 0.18));
        painter.drawEllipse(QRectF(rect.center().x() - rect.width() * 0.10, rect.bottom() - rect.height() * 0.36, rect.width() * 0.20, rect.height() * 0.20));
        return pixmap;
    }

    if (kind == GlyphKind::House) {
        painter.setBrush(color);
        QPainterPath house;
        house.moveTo(rect.left() + rect.width() * 0.12, rect.top() + rect.height() * 0.46);
        house.lineTo(rect.center().x(), rect.top() + rect.height() * 0.16);
        house.lineTo(rect.right() - rect.width() * 0.12, rect.top() + rect.height() * 0.46);
        house.lineTo(rect.right() - rect.width() * 0.12, rect.bottom() - rect.height() * 0.12);
        house.lineTo(rect.left() + rect.width() * 0.12, rect.bottom() - rect.height() * 0.12);
        house.closeSubpath();
        painter.drawPath(house);
        painter.setBrush(QColor(255, 255, 255, 120));
        painter.drawRect(QRectF(rect.center().x() - rect.width() * 0.08, rect.bottom() - rect.height() * 0.34, rect.width() * 0.16, rect.height() * 0.22));
        return pixmap;
    }

    painter.setBrush(color);
    painter.drawRect(QRectF(rect.left() + rect.width() * 0.18, rect.top() + rect.height() * 0.18, rect.width() * 0.64, rect.height() * 0.64));
    painter.setBrush(QColor(255, 255, 255, 120));
    painter.drawRect(QRectF(rect.left() + rect.width() * 0.28, rect.top() + rect.height() * 0.28, rect.width() * 0.14, rect.height() * 0.12));
    painter.drawRect(QRectF(rect.left() + rect.width() * 0.50, rect.top() + rect.height() * 0.28, rect.width() * 0.14, rect.height() * 0.12));
    painter.drawRect(QRectF(rect.left() + rect.width() * 0.28, rect.top() + rect.height() * 0.50, rect.width() * 0.14, rect.height() * 0.12));
    painter.drawRect(QRectF(rect.left() + rect.width() * 0.50, rect.top() + rect.height() * 0.50, rect.width() * 0.14, rect.height() * 0.12));
    return pixmap;
}

void configureActionButton(QToolButton* button, const QString& text, GlyphKind kind)
{
    button->setText(text);
    button->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    button->setIcon(QIcon(makeGlyphPixmap(kind, QSize(28, 28))));
    button->setIconSize(QSize(28, 28));
    button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

}  // namespace

GameWindow::GameWindow(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("gameWindow"));
    setMinimumSize(1180, 760);

    auto *rootLayout = new QHBoxLayout(this);
    rootLayout->setContentsMargins(18, 18, 18, 18);
    rootLayout->setSpacing(16);

    auto *boardShell = new QFrame(this);
    boardShell->setObjectName(QStringLiteral("boardShell"));
    auto *boardLayout = new QVBoxLayout(boardShell);
    boardLayout->setContentsMargins(18, 18, 18, 18);
    boardLayout->setSpacing(0);

    boardWidget = new BoardWidget(boardShell);
    boardWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    boardLayout->addWidget(boardWidget, 1);
    rootLayout->addWidget(boardShell, 1);

    sidebarPanel = new QFrame(this);
    sidebarPanel->setObjectName(QStringLiteral("sidebarPanel"));
    sidebarPanel->setMinimumWidth(300);
    sidebarPanel->setMaximumWidth(330);

    auto *sidebarLayout = new QVBoxLayout(sidebarPanel);
    sidebarLayout->setContentsMargins(0, 0, 0, 0);
    sidebarLayout->setSpacing(0);
    rootLayout->addWidget(sidebarPanel, 0);

    auto *playerHeader = new QFrame(sidebarPanel);
    playerHeader->setObjectName(QStringLiteral("playerHeader"));
    auto *playerHeaderLayout = new QHBoxLayout(playerHeader);
    playerHeaderLayout->setContentsMargins(18, 16, 18, 14);
    playerHeaderLayout->setSpacing(12);

    playerAvatarLabel = new QLabel(playerHeader);
    playerAvatarLabel->setObjectName(QStringLiteral("playerAvatar"));
    playerAvatarLabel->setFixedSize(64, 64);
    playerAvatarLabel->setAlignment(Qt::AlignCenter);
    playerHeaderLayout->addWidget(playerAvatarLabel, 0, Qt::AlignTop);

    auto *playerTextColumn = new QVBoxLayout();
    playerTextColumn->setSpacing(4);
    playerHeaderLayout->addLayout(playerTextColumn, 1);

    playerNameLabel = new QLabel(QStringLiteral("Player"), playerHeader);
    playerNameLabel->setObjectName(QStringLiteral("playerName"));
    playerTextColumn->addWidget(playerNameLabel);

    playerMoneyLabel = new QLabel(QStringLiteral("M0"), playerHeader);
    playerMoneyLabel->setObjectName(QStringLiteral("playerMoney"));
    playerTextColumn->addWidget(playerMoneyLabel);

    playerMetaLabel = new QLabel(QStringLiteral("Detail pemain akan muncul di sini."), playerHeader);
    playerMetaLabel->setObjectName(QStringLiteral("playerMeta"));
    playerMetaLabel->setWordWrap(true);
    playerMetaLabel->setVisible(false);
    playerTextColumn->addWidget(playerMetaLabel);

    sidebarLayout->addWidget(playerHeader, 0);

    auto *selectorWrap = new QFrame(sidebarPanel);
    selectorWrap->setObjectName(QStringLiteral("selectorWrap"));
    auto *selectorLayout = new QHBoxLayout(selectorWrap);
    selectorLayout->setContentsMargins(18, 10, 18, 4);
    selectorLayout->setSpacing(8);

    auto *selectorContainer = new QWidget(selectorWrap);
    playerSelectorLayout = new QHBoxLayout(selectorContainer);
    playerSelectorLayout->setContentsMargins(0, 0, 0, 0);
    playerSelectorLayout->setSpacing(10);
    selectorLayout->addWidget(selectorContainer, 0);
    selectorLayout->addStretch(1);

    playerSelectorGroup = new QButtonGroup(this);
    playerSelectorGroup->setExclusive(true);
    sidebarLayout->addWidget(selectorWrap, 0);

    auto *portfolioSection = new QFrame(sidebarPanel);
    portfolioSection->setObjectName(QStringLiteral("portfolioSection"));
    auto *portfolioLayout = new QVBoxLayout(portfolioSection);
    portfolioLayout->setContentsMargins(18, 4, 18, 4);
    portfolioLayout->setSpacing(0);

    portfolioWidget = new PropertyPortfolioWidget(portfolioSection);
    portfolioWidget->setObjectName(QStringLiteral("portfolioWidget"));
    portfolioWidget->setFixedHeight(208);
    portfolioLayout->addWidget(portfolioWidget, 1);
    sidebarLayout->addWidget(portfolioSection, 0);

    auto *divider = new QFrame(sidebarPanel);
    divider->setObjectName(QStringLiteral("sidebarDivider"));
    divider->setFixedHeight(1);
    sidebarLayout->addWidget(divider, 0);

    auto *statsSection = new QFrame(sidebarPanel);
    statsSection->setObjectName(QStringLiteral("statsSection"));
    auto *statsLayout = new QHBoxLayout(statsSection);
    statsLayout->setContentsMargins(18, 8, 18, 10);
    statsLayout->setSpacing(10);

    auto *houseIconLabel = new QLabel(statsSection);
    houseIconLabel->setPixmap(makeGlyphPixmap(GlyphKind::House, QSize(24, 24), QColor(47, 134, 235)));
    statsLayout->addWidget(houseIconLabel, 0, Qt::AlignVCenter);

    houseCountLabel = new QLabel(QStringLiteral("0"), statsSection);
    houseCountLabel->setObjectName(QStringLiteral("statLabelPrimary"));
    statsLayout->addWidget(houseCountLabel, 0, Qt::AlignVCenter);

    auto *hotelIconLabel = new QLabel(statsSection);
    hotelIconLabel->setPixmap(makeGlyphPixmap(GlyphKind::Hotel, QSize(24, 24), QColor(67, 76, 96)));
    statsLayout->addWidget(hotelIconLabel, 0, Qt::AlignVCenter);

    hotelCountLabel = new QLabel(QStringLiteral("0"), statsSection);
    hotelCountLabel->setObjectName(QStringLiteral("statLabelSecondary"));
    statsLayout->addWidget(hotelCountLabel, 0, Qt::AlignVCenter);

    statsLayout->addStretch(1);
    sidebarLayout->addWidget(statsSection, 0);

    auto *actionsSection = new QFrame(sidebarPanel);
    actionsSection->setObjectName(QStringLiteral("actionsSection"));
    auto *actionsLayout = new QGridLayout(actionsSection);
    actionsLayout->setContentsMargins(18, 0, 18, 12);
    actionsLayout->setHorizontalSpacing(10);
    actionsLayout->setVerticalSpacing(10);

    rollButton = new QToolButton(actionsSection);
    rollButton->setObjectName(QStringLiteral("actionButtonPrimary"));
    rollButton->setCursor(Qt::PointingHandCursor);
    rollButton->setMinimumHeight(68);
    configureActionButton(rollButton, QStringLiteral("ROLL DICE"), GlyphKind::Dice);

    buildButton = new QToolButton(actionsSection);
    buildButton->setObjectName(QStringLiteral("actionButton"));
    buildButton->setCursor(Qt::PointingHandCursor);
    buildButton->setMinimumHeight(68);
    configureActionButton(buildButton, QStringLiteral("BUILD"), GlyphKind::Build);

    mortgageButton = new QToolButton(actionsSection);
    mortgageButton->setObjectName(QStringLiteral("actionButton"));
    mortgageButton->setCursor(Qt::PointingHandCursor);
    mortgageButton->setMinimumHeight(68);
    configureActionButton(mortgageButton, QStringLiteral("MORTGAGE"), GlyphKind::Mortgage);

    saveButton = new QToolButton(actionsSection);
    saveButton->setObjectName(QStringLiteral("actionButton"));
    saveButton->setCursor(Qt::PointingHandCursor);
    saveButton->setMinimumHeight(68);
    configureActionButton(saveButton, QStringLiteral("SAVE"), GlyphKind::Save);

    actionsLayout->addWidget(rollButton, 0, 0);
    actionsLayout->addWidget(buildButton, 0, 1);
    actionsLayout->addWidget(mortgageButton, 1, 0);
    actionsLayout->addWidget(saveButton, 1, 1);
    sidebarLayout->addWidget(actionsSection, 0);

    auto *historySection = new QFrame(sidebarPanel);
    historySection->setObjectName(QStringLiteral("historySection"));
    auto *historySectionLayout = new QVBoxLayout(historySection);
    historySectionLayout->setContentsMargins(18, 0, 18, 18);
    historySectionLayout->setSpacing(0);

    historyHeaderFrame = new QFrame(historySection);
    historyHeaderFrame->setObjectName(QStringLiteral("historyHeaderFrame"));
    auto *historyHeaderLayout = new QHBoxLayout(historyHeaderFrame);
    historyHeaderLayout->setContentsMargins(10, 8, 10, 8);
    historyHeaderLayout->setSpacing(6);

    auto *historyTitle = new QLabel(QStringLiteral("HISTORY"), historyHeaderFrame);
    historyTitle->setObjectName(QStringLiteral("historyTitle"));
    historyHeaderLayout->addWidget(historyTitle, 0, Qt::AlignVCenter);
    historyHeaderLayout->addStretch(1);

    auto *historyFilter = new QLabel(QStringLiteral("---"), historyHeaderFrame);
    historyFilter->setObjectName(QStringLiteral("historyFilter"));
    historyHeaderLayout->addWidget(historyFilter, 0, Qt::AlignVCenter);
    historySectionLayout->addWidget(historyHeaderFrame, 0);

    auto *historyScroll = new QScrollArea(historySection);
    historyScroll->setObjectName(QStringLiteral("historyScroll"));
    historyScroll->setWidgetResizable(true);
    historyScroll->setFrameShape(QFrame::NoFrame);
    historyScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    historyScroll->setMinimumHeight(210);

    auto *historyContent = new QWidget(historyScroll);
    historyEntriesLayout = new QVBoxLayout(historyContent);
    historyEntriesLayout->setContentsMargins(0, 0, 0, 0);
    historyEntriesLayout->setSpacing(0);
    historyEntriesLayout->addStretch(1);
    historyScroll->setWidget(historyContent);
    historySectionLayout->addWidget(historyScroll, 1);
    sidebarLayout->addWidget(historySection, 1);

    propertyCardDialog = new QDialog(this, Qt::Dialog | Qt::WindowCloseButtonHint | Qt::WindowTitleHint);
    propertyCardDialog->setModal(false);
    propertyCardDialog->setWindowTitle(QStringLiteral("Property Card"));
    propertyCardDialog->resize(410, 620);
    auto *dialogLayout = new QVBoxLayout(propertyCardDialog);
    dialogLayout->setContentsMargins(12, 12, 12, 12);
    propertyCardWidget = new PropertyCardWidget(propertyCardDialog);
    dialogLayout->addWidget(propertyCardWidget);

    setStyleSheet(
        "#gameWindow {"
        "  background: #eef2f5;"
        "}"
        "#boardShell {"
        "  background: #ffffff;"
        "  border: 1px solid #d6dde3;"
        "  border-radius: 18px;"
        "}"
        "#sidebarPanel {"
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #a7d0e1, stop:1 #a0cadb);"
        "  border: 1px solid #7faec0;"
        "  border-radius: 20px;"
        "}"
        "#playerHeader {"
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 rgba(186, 220, 235, 240), stop:1 rgba(168, 207, 225, 244));"
        "  border-bottom: 1px solid rgba(112, 150, 165, 0.55);"
        "  border-top-left-radius: 20px;"
        "  border-top-right-radius: 20px;"
        "}"
        "#selectorWrap, #portfolioSection, #statsSection, #actionsSection, #historySection {"
        "  background: transparent;"
        "}"
        "#sidebarDivider {"
        "  background: rgba(111, 150, 167, 0.55);"
        "  margin: 6px 18px 4px 18px;"
        "}"
        "#playerAvatar {"
        "  background: rgba(255, 255, 255, 0.82);"
        "  border: 2px solid rgba(255, 255, 255, 0.95);"
        "  border-radius: 11px;"
        "}"
        "#playerName {"
        "  color: #10181f;"
        "  font: 900 15pt 'Arial';"
        "}"
        "#playerMoney {"
        "  color: #2f8c2f;"
        "  font: 900 18pt 'Arial';"
        "}"
        "#playerMeta {"
        "  color: #344b59;"
        "  font: 600 7.5pt 'Arial';"
        "}"
        "#sectionLabel {"
        "  color: #27404d;"
        "  font: 700 8.5pt 'Arial';"
        "}"
        "#sectionTitle {"
        "  color: #1b2f39;"
        "  font: 800 10.5pt 'Arial';"
        "}"
        "#historyTitle {"
        "  color: #243744;"
        "  font: 900 8.5pt 'Arial';"
        "  letter-spacing: 1px;"
        "}"
        "QPushButton[playerChip='true'] {"
        "  min-width: 48px;"
        "  min-height: 32px;"
        "  padding: 5px 12px;"
        "  border-radius: 16px;"
        "  border: 1px solid rgba(78, 119, 136, 0.46);"
        "  background: rgba(255, 255, 255, 0.38);"
        "  color: #244150;"
        "  font: 800 8pt 'Arial';"
        "}"
        "QPushButton[playerChip='true']:checked {"
        "  background: rgba(255, 255, 255, 0.92);"
        "  border: 2px solid #347cb7;"
        "  color: #122735;"
        "}"
        "#statLabelPrimary, #statLabelSecondary {"
        "  color: #245987;"
        "  font: 900 10pt 'Arial';"
        "}"
        "#statLabelSecondary {"
        "  color: #3f7a33;"
        "}"
        "#actionButtonPrimary, #actionButton {"
        "  padding: 6px 12px;"
        "  border-radius: 10px;"
        "  border: 1px solid rgba(134, 152, 166, 0.9);"
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #ffffff, stop:1 #dfe7ec);"
        "  color: #162432;"
        "  font: 900 10pt 'Arial';"
        "}"
        "#actionButtonPrimary:hover, #actionButton:hover {"
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #ffffff, stop:1 #edf4f7);"
        "}"
        "#actionButtonPrimary:pressed, #actionButton:pressed {"
        "  padding-top: 8px;"
        "}"
        "#historyHeaderFrame {"
        "  background: rgba(255, 255, 255, 0.92);"
        "  border: 1px solid rgba(138, 160, 174, 0.95);"
        "  border-top-left-radius: 10px;"
        "  border-top-right-radius: 10px;"
        "}"
        "#historyScroll {"
        "  background: rgba(255,255,255,0.82);"
        "  border-left: 1px solid rgba(138, 160, 174, 0.95);"
        "  border-right: 1px solid rgba(138, 160, 174, 0.95);"
        "  border-bottom: 1px solid rgba(138, 160, 174, 0.95);"
        "  border-bottom-left-radius: 10px;"
        "  border-bottom-right-radius: 10px;"
        "}"
        "#historyFilter {"
        "  color: #596d7d;"
        "  font: 700 8pt 'Courier New';"
        "}"
        "QDialog {"
        "  background: #d9d4c5;"
        "}"
    );

    loadConfigData();
    initializeGame();

    connect(boardWidget, &BoardWidget::propertySelected, this, [this](int propertyId) {
        const DemoPropertyState* propertyState = propertyStateForId(propertyId);
        if (propertyState != nullptr) {
            if (!propertyState->ownerUsername.isEmpty()) {
                setSelectedPlayer(propertyState->ownerUsername);
            }
        }
        setSelectedProperty(propertyId, true);
    });

    connect(portfolioWidget, &PropertyPortfolioWidget::propertyActivated, this, [this](int propertyId) {
        setSelectedProperty(propertyId, true);
    });

    connect(playerSelectorGroup, QOverload<QAbstractButton *>::of(&QButtonGroup::buttonClicked), this, [this](QAbstractButton *button) {
        if (button == nullptr) {
            return;
        }
        setSelectedPlayer(button->property("username").toString());
    });

    connect(rollButton, &QToolButton::clicked, this, [this]() {
        handleRollRequested();
    });

    connect(saveButton, &QToolButton::clicked, this, [this]() {
        saveCurrentGame();
    });

    connect(buildButton, &QToolButton::clicked, this, [this]() {
        QMessageBox::information(
            this,
            QStringLiteral("Build"),
            QStringLiteral("Tombol BUILD sudah dipasang di layout baru. Aksi backend build belum kita sambungkan ke flow GUI ini.")
        );
    });

    connect(mortgageButton, &QToolButton::clicked, this, [this]() {
        QMessageBox::information(
            this,
            QStringLiteral("Mortgage"),
            QStringLiteral("Tombol MORTGAGE sudah dipasang di layout baru. Aksi backend mortgage belum kita sambungkan ke flow GUI ini.")
        );
    });
}

void GameWindow::loadConfigData()
{
    properties.clear();
    configData = ConfigData();
    hasConfigData = false;

    const QString configDir = MonopolyUi::findConfigDirectory();
    propertyCardWidget->setConfigData(configData);
    if (configDir.isEmpty()) {
        return;
    }

    try {
        ConfigLoader loader(configDir.toStdString());
        configData = loader.loadAll();
        properties = configData.getPropertyConfigs();
        hasConfigData = true;
        propertyCardWidget->setConfigData(configData);
    } catch (const std::exception&) {
        properties.clear();
        configData = ConfigData();
        propertyCardWidget->setConfigData(configData);
    }
}

void GameWindow::initializeGame()
{
    pawnAssetByPlayer.clear();
    accentColorByPlayer.clear();
    playerOverviews.clear();
    propertyStateById.clear();
    historyEntries.clear();
    activePlayerUsername.clear();
    currentTurn = 1;

    pawnAssetByPlayer.insert(QStringLiteral("Player ITB"), QStringLiteral("ITB.png"));
    pawnAssetByPlayer.insert(QStringLiteral("Player UGM"), QStringLiteral("UGM.avif"));
    pawnAssetByPlayer.insert(QStringLiteral("Player UI"), QStringLiteral("UI.jpg"));

    accentColorByPlayer.insert(QStringLiteral("Player ITB"), QColor(255, 120, 90));
    accentColorByPlayer.insert(QStringLiteral("Player UGM"), QColor(94, 193, 255));
    accentColorByPlayer.insert(QStringLiteral("Player UI"), QColor(86, 214, 150));

    playerOverviews = {
        {QStringLiteral("Player ITB"), pawnAssetNameForPlayer(QStringLiteral("Player ITB")), accentColorForPlayer(QStringLiteral("Player ITB")), 1000, 0, {}, true, false},
        {QStringLiteral("Player UGM"), pawnAssetNameForPlayer(QStringLiteral("Player UGM")), accentColorForPlayer(QStringLiteral("Player UGM")), 1000, 10, {}, false, false},
        {QStringLiteral("Player UI"), pawnAssetNameForPlayer(QStringLiteral("Player UI")), accentColorForPlayer(QStringLiteral("Player UI")), 1000, 20, {}, false, false}
    };

    activePlayerUsername = playerOverviews.front().name;
    selectedPlayerUsername = activePlayerUsername;
    lastStatusText = QStringLiteral("Mode GUI mandiri aktif. Integrasi gameplay backend sedang dimatikan sementara.");

    const auto assignProperty = [this](int propertyId, const QString& owner, int buildingLevel = 0, bool mortgaged = false) {
        DemoPropertyState state;
        state.ownerUsername = owner;
        state.buildingLevel = buildingLevel;
        state.mortgaged = mortgaged;
        propertyStateById.insert(propertyId, state);
    };

    assignProperty(2, QStringLiteral("Player ITB"), 1);
    assignProperty(4, QStringLiteral("Player ITB"));
    assignProperty(7, QStringLiteral("Player ITB"));
    assignProperty(9, QStringLiteral("Player ITB"));
    assignProperty(10, QStringLiteral("Player ITB"));
    assignProperty(12, QStringLiteral("Player ITB"), 1);
    assignProperty(14, QStringLiteral("Player ITB"));
    assignProperty(17, QStringLiteral("Player ITB"), 2);
    assignProperty(18, QStringLiteral("Player ITB"), 2);
    assignProperty(19, QStringLiteral("Player ITB"), 1);
    assignProperty(21, QStringLiteral("Player ITB"), 1);
    assignProperty(26, QStringLiteral("Player ITB"), 1);
    assignProperty(29, QStringLiteral("Player ITB"), 1);
    assignProperty(31, QStringLiteral("Player ITB"));
    assignProperty(35, QStringLiteral("Player ITB"));
    assignProperty(39, QStringLiteral("Player ITB"));

    assignProperty(1, QStringLiteral("Player UGM"));
    assignProperty(3, QStringLiteral("Player UGM"));
    assignProperty(6, QStringLiteral("Player UGM"), 1);
    assignProperty(8, QStringLiteral("Player UGM"));
    assignProperty(11, QStringLiteral("Player UGM"));
    assignProperty(13, QStringLiteral("Player UGM"));
    assignProperty(15, QStringLiteral("Player UGM"));
    assignProperty(25, QStringLiteral("Player UGM"));
    assignProperty(28, QStringLiteral("Player UGM"));

    assignProperty(5, QStringLiteral("Player UI"));
    assignProperty(16, QStringLiteral("Player UI"));
    assignProperty(23, QStringLiteral("Player UI"));
    assignProperty(24, QStringLiteral("Player UI"));
    assignProperty(27, QStringLiteral("Player UI"));
    assignProperty(37, QStringLiteral("Player UI"));

    historyEntries = {
        {1, QStringLiteral("SYSTEM"), QStringLiteral("SYSTEM"), QStringLiteral("Mode GUI berdiri sendiri aktif. Data pemain memakai mock state sementara.")},
        {1, QStringLiteral("Player ITB"), QStringLiteral("BELI"), QStringLiteral("Menguasai beberapa properti contoh untuk preview portfolio.")},
        {1, QStringLiteral("Player UGM"), QStringLiteral("BELI"), QStringLiteral("Memiliki beberapa properti agar selector pemain tetap informatif.")},
        {1, QStringLiteral("Player UI"), QStringLiteral("LEWAT"), QStringLiteral("Belum ada aksi tambahan pada giliran ini.")}
    };

    refreshScene(lastStatusText);
    if (!properties.empty()) {
        setSelectedProperty(properties.front().getId(), false);
    }
}

void GameWindow::refreshSidebar()
{
    refreshPlayerHeader();
    refreshPlayerSelector();
    refreshPortfolio();
    refreshStats();
    refreshHistory();
}

void GameWindow::refreshPlayerHeader()
{
    const PlayerOverview* player = playerOverviewByUsername(selectedPlayerUsername);
    if (player == nullptr) {
        playerNameLabel->setText(QStringLiteral("Belum ada pemain"));
        playerMoneyLabel->setText(QStringLiteral("M0"));
        playerMetaLabel->setText(lastStatusText);
        playerAvatarLabel->setText(QStringLiteral("--"));
        playerAvatarLabel->setPixmap(QPixmap());
        return;
    }

    playerNameLabel->setText(player->name);
    playerMoneyLabel->setText(MonopolyUi::formatCurrency(player->balance));
    playerMetaLabel->clear();

    const QPixmap pawn = loadPawnPixmap(player->pawnAssetName, QSize(56, 56));
    if (!pawn.isNull()) {
        playerAvatarLabel->setStyleSheet(QString());
        playerAvatarLabel->setText(QString());
        playerAvatarLabel->setPixmap(pawn);
    } else {
        playerAvatarLabel->setPixmap(QPixmap());
        playerAvatarLabel->setText(shortPlayerLabel(player->name));
        playerAvatarLabel->setStyleSheet(QStringLiteral(
            "background: rgba(255,255,255,0.82);"
            "border: 2px solid rgba(255,255,255,0.95);"
            "border-radius: 11px;"
            "color: #203240;"
            "font: 900 15pt 'Arial';"
        ));
    }
}

void GameWindow::refreshPlayerSelector()
{
    while (QLayoutItem *item = playerSelectorLayout->takeAt(0)) {
        if (item->widget() != nullptr) {
            playerSelectorGroup->removeButton(qobject_cast<QAbstractButton*>(item->widget()));
            item->widget()->deleteLater();
        }
        delete item;
    }

    int buttonIndex = 0;
    for (const PlayerOverview& player : playerOverviews) {
        auto *button = new QPushButton(shortPlayerLabel(player.name), sidebarPanel);
        button->setCheckable(true);
        button->setProperty("playerChip", true);
        button->setProperty("username", player.name);
        button->setChecked(player.name == selectedPlayerUsername);
        playerSelectorLayout->addWidget(button);
        playerSelectorGroup->addButton(button, buttonIndex++);
    }
    playerSelectorLayout->addStretch(1);
}

void GameWindow::refreshPortfolio()
{
    portfolioWidget->setPortfolio(buildPortfolioForPlayer(selectedPlayerUsername));
    portfolioWidget->setSelectedPropertyId(selectedPropertyId);
}

void GameWindow::refreshStats()
{
    houseCountLabel->setText(QString::number(totalHouseCount(selectedPlayerUsername)));
    hotelCountLabel->setText(QString::number(totalHotelCount(selectedPlayerUsername)));
}

void GameWindow::refreshHistory()
{
    while (QLayoutItem *item = historyEntriesLayout->takeAt(0)) {
        if (item->widget() != nullptr) {
            item->widget()->deleteLater();
        }
        delete item;
    }

    if (historyEntries.isEmpty()) {
        auto *emptyLabel = new QLabel(QStringLiteral("Belum ada transaksi yang tercatat."), sidebarPanel);
        emptyLabel->setWordWrap(true);
        emptyLabel->setStyleSheet(QStringLiteral("color:#324b58;font:600 8pt 'Arial';padding:12px 10px;"));
        historyEntriesLayout->addWidget(emptyLabel);
        historyEntriesLayout->addStretch(1);
        return;
    }

    for (auto it = historyEntries.crbegin(); it != historyEntries.crend(); ++it) {
        auto *entryFrame = new QFrame(sidebarPanel);
        entryFrame->setStyleSheet(QStringLiteral(
            "background: rgba(255,255,255,0.78);"
            "border-left: 4px solid %1;"
            "border-bottom: 1px solid rgba(211, 221, 226, 0.95);"
        ).arg(historyAccentColor(*it)));

        auto *entryLayout = new QVBoxLayout(entryFrame);
        entryLayout->setContentsMargins(10, 8, 10, 8);
        entryLayout->setSpacing(3);

        auto *metaLabel = new QLabel(
            QStringLiteral("Turn %1 | %2 | %3")
                .arg(it->turn)
                .arg(it->username)
                .arg(it->actionType),
            entryFrame
        );
        metaLabel->setStyleSheet(QStringLiteral("color:#8294a1;font:700 7pt 'Arial';"));
        entryLayout->addWidget(metaLabel);

        auto *detailLabel = new QLabel(it->detail, entryFrame);
        detailLabel->setWordWrap(true);
        detailLabel->setStyleSheet(QStringLiteral("color:#17232d;font:700 8.5pt 'Arial';"));
        entryLayout->addWidget(detailLabel);

        historyEntriesLayout->addWidget(entryFrame);
    }

    historyEntriesLayout->addStretch(1);
}

void GameWindow::refreshBoardPawns()
{
    QVector<BoardWidget::PawnData> pawnData;
    pawnData.reserve(playerOverviews.size());

    for (const PlayerOverview& player : playerOverviews) {
        pawnData.append({player.name, player.pawnAssetName, player.tileIndex, player.accentColor});
    }

    boardWidget->setPawns(pawnData);
    boardWidget->setActivePawnName(activePlayerUsername);
}

void GameWindow::syncSelectedPlayer()
{
    if (!selectedPlayerUsername.isEmpty() && playerOverviewByUsername(selectedPlayerUsername) != nullptr) {
        return;
    }

    if (!activePlayerUsername.isEmpty() && playerOverviewByUsername(activePlayerUsername) != nullptr) {
        selectedPlayerUsername = activePlayerUsername;
        return;
    }

    if (!playerOverviews.isEmpty()) {
        selectedPlayerUsername = playerOverviews.front().name;
    }
}

void GameWindow::setSelectedPlayer(const QString& username)
{
    if (username.isEmpty() || playerOverviewByUsername(username) == nullptr) {
        return;
    }

    selectedPlayerUsername = username;
    refreshSidebar();
}

void GameWindow::setSelectedProperty(int propertyId, bool showPreview)
{
    selectedPropertyId = propertyId;
    boardWidget->setSelectedPropertyId(propertyId);
    portfolioWidget->setSelectedPropertyId(propertyId);
    propertyCardWidget->setSelectedProperty(propertyId);

    if (showPreview) {
        showPropertyCard(propertyId);
    }
}

void GameWindow::showPropertyCard(int propertyId)
{
    if (propertyConfigForId(propertyId) == nullptr) {
        return;
    }

    propertyCardWidget->setSelectedProperty(propertyId);
    if (!propertyCardDialog->isVisible()) {
        const QPoint anchor = mapToGlobal(rect().center());
        propertyCardDialog->move(anchor.x() - propertyCardDialog->width() / 2, anchor.y() - propertyCardDialog->height() / 2);
    }
    propertyCardDialog->show();
    propertyCardDialog->raise();
    propertyCardDialog->activateWindow();
}

void GameWindow::handleRollRequested()
{
    if (playerOverviews.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("UI"), QStringLiteral("Belum ada data pemain untuk mode GUI mandiri."));
        return;
    }

    PlayerOverview* activePlayer = playerOverviewByUsername(activePlayerUsername);
    if (activePlayer == nullptr) {
        return;
    }

    const int dieOne = QRandomGenerator::global()->bounded(1, 7);
    const int dieTwo = QRandomGenerator::global()->bounded(1, 7);
    const int total = dieOne + dieTwo;
    const int previousPosition = activePlayer->tileIndex;
    activePlayer->tileIndex = (activePlayer->tileIndex + total) % 40;
    if (previousPosition + total >= 40) {
        activePlayer->balance += 200;
    }

    lastStatusText = QStringLiteral("%1 melempar %2 + %3 dan berhenti di petak %4.")
        .arg(activePlayer->name)
        .arg(dieOne)
        .arg(dieTwo)
        .arg(activePlayer->tileIndex + 1);

    historyEntries.append({currentTurn, activePlayer->name, QStringLiteral("ROLL"), lastStatusText});

    const int landedPropertyId = activePlayer->tileIndex + 1;
    if (propertyConfigForId(landedPropertyId) != nullptr) {
        setSelectedProperty(landedPropertyId, false);
    }

    int activeIndex = 0;
    for (int i = 0; i < playerOverviews.size(); ++i) {
        if (playerOverviews[i].name == activePlayerUsername) {
            activeIndex = i;
            break;
        }
    }

    for (PlayerOverview& player : playerOverviews) {
        player.isCurrentTurn = false;
    }

    activeIndex = (activeIndex + 1) % playerOverviews.size();
    activePlayerUsername = playerOverviews[activeIndex].name;
    playerOverviews[activeIndex].isCurrentTurn = true;
    ++currentTurn;

    refreshScene(lastStatusText);
}

void GameWindow::saveCurrentGame()
{
    QMessageBox::information(
        this,
        QStringLiteral("Save Dinonaktifkan"),
        QStringLiteral("Integrasi penyimpanan GUI sengaja dimatikan sementara sampai file logic/game state selesai dirapikan.")
    );
}

void GameWindow::refreshScene(const QString& statusText)
{
    if (!statusText.isEmpty()) {
        lastStatusText = statusText;
    }

    syncSelectedPlayer();
    refreshBoardPawns();
    refreshSidebar();

    if (selectedPropertyId > 0) {
        boardWidget->setSelectedPropertyId(selectedPropertyId);
        portfolioWidget->setSelectedPropertyId(selectedPropertyId);
    }
}

PlayerOverview* GameWindow::playerOverviewByUsername(const QString& username)
{
    for (PlayerOverview& player : playerOverviews) {
        if (player.name == username) {
            return &player;
        }
    }

    return nullptr;
}

const PlayerOverview* GameWindow::playerOverviewByUsername(const QString& username) const
{
    for (const PlayerOverview& player : playerOverviews) {
        if (player.name == username) {
            return &player;
        }
    }

    return nullptr;
}

const PropertyConfig* GameWindow::propertyConfigForId(int propertyId) const
{
    for (const PropertyConfig& property : properties) {
        if (property.getId() == propertyId) {
            return &property;
        }
    }

    return nullptr;
}

const DemoPropertyState* GameWindow::propertyStateForId(int propertyId) const
{
    const auto it = propertyStateById.constFind(propertyId);
    if (it == propertyStateById.constEnd()) {
        return nullptr;
    }

    return &it.value();
}

QVector<int> GameWindow::ownedPropertyIds(const QString& username) const
{
    QVector<int> result;
    for (auto it = propertyStateById.constBegin(); it != propertyStateById.constEnd(); ++it) {
        if (it.value().ownerUsername == username) {
            result.append(it.key());
        }
    }

    std::sort(result.begin(), result.end());
    return result;
}

QVector<PortfolioPropertyView> GameWindow::buildPortfolioForPlayer(const QString& username) const
{
    QVector<PortfolioPropertyView> result;
    result.reserve(static_cast<int>(properties.size()));

    for (const PropertyConfig& property : properties) {
        PortfolioPropertyView entry;
        entry.propertyId = property.getId();
        entry.code = QString::fromStdString(property.getCode());
        entry.title = MonopolyUi::singleLineTileName(property.getName());
        entry.propertyType = property.getPropertyType();
        entry.colorGroup = property.getColorGroup();

        const DemoPropertyState* state = propertyStateForId(property.getId());
        if (state != nullptr) {
            entry.owned = state->ownerUsername == username;
            entry.mortgaged = state->mortgaged;
            entry.buildingLevel = state->buildingLevel;
        }

        result.append(entry);
    }

    return result;
}

QString GameWindow::pawnAssetNameForPlayer(const QString& username) const
{
    return pawnAssetByPlayer.value(username, QStringLiteral("fallback_pawn.png"));
}

QColor GameWindow::accentColorForPlayer(const QString& username) const
{
    return accentColorByPlayer.value(username, QColor(90, 190, 240));
}

int GameWindow::totalHouseCount(const QString& username) const
{
    int houses = 0;
    for (auto it = propertyStateById.constBegin(); it != propertyStateById.constEnd(); ++it) {
        if (it.value().ownerUsername != username) {
            continue;
        }

        houses += std::min(it.value().buildingLevel, 4);
    }

    return houses;
}

int GameWindow::totalHotelCount(const QString& username) const
{
    int hotels = 0;
    for (auto it = propertyStateById.constBegin(); it != propertyStateById.constEnd(); ++it) {
        if (it.value().ownerUsername == username && it.value().buildingLevel >= 5) {
            ++hotels;
        }
    }

    return hotels;
}
