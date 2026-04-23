#include "views/GameWindow.hpp"

#include <algorithm>
#include <cctype>
#include <functional>
#include <vector>

#include <QApplication>
#include <QAction>
#include <QDialog>
#include <QDir>
#include <QEventLoop>
#include <QFileInfo>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLayoutItem>
#include <QLineEdit>
#include <QMenu>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QSet>
#include <QSpinBox>
#include <QStackedWidget>
#include <QStyle>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

#include "models/state/GameState.hpp"
#include "models/state/LogEntry.hpp"
#include "models/state/PropertyState.hpp"
#include "models/tiles/PropertyTile.hpp"
#include "models/tiles/Tile.hpp"
#include "utils/SaveManager.hpp"
#include "utils/UiCommon.hpp"
#include "views/BoardWidget.hpp"
#include "views/GameSetupPage.hpp"
#include "views/PropertyCardWidget.hpp"
#include "views/PropertyPortfolioWidget.hpp"
#include "views/StartMenuPage.hpp"

namespace {

QString shortPlayerLabel(const QString& username)
{
    const QStringList pieces = username.split(QRegularExpression(QStringLiteral("[_\\s]+")), Qt::SkipEmptyParts);
    if (pieces.isEmpty()) {
        return username.left(2).toUpper();
    }

    if (pieces.size() == 1) {
        return pieces.front().left(2).toUpper();
    }

    return (pieces.front().left(1) + pieces.back().left(1)).toUpper();
}

QString historyAccentColor(const HistoryEntryView& entry)
{
    const QString action = entry.actionType.toUpper();
    if (action == QStringLiteral("SEWA") || action == QStringLiteral("PAJAK")) {
        return QStringLiteral("#df5c58");
    }
    if (action == QStringLiteral("BELI") || action == QStringLiteral("LELANG")) {
        return QStringLiteral("#f38a2d");
    }
    if (action == QStringLiteral("SYSTEM") || action == QStringLiteral("LOAD") || action == QStringLiteral("SAVE")) {
        return QStringLiteral("#2e8ef7");
    }
    if (action == QStringLiteral("KARTU") || action == QStringLiteral("KARTU_SKILL")) {
        return QStringLiteral("#7a49d8");
    }
    return QStringLiteral("#48a45b");
}

QColor playerAccent(int index)
{
    switch (index) {
    case 0:
        return QColor(28, 97, 214);
    case 1:
        return QColor(216, 40, 40);
    case 2:
        return QColor(36, 38, 42);
    default:
        return QColor(126, 130, 138);
    }
}

void configureActionButton(QToolButton* button, const QString& text, const QIcon& icon, bool primary = false)
{
    button->setText(text);
    button->setIcon(icon);
    button->setIconSize(QSize(26, 26));
    button->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    button->setCursor(Qt::PointingHandCursor);
    button->setMinimumHeight(68);
    button->setObjectName(primary ? QStringLiteral("actionButtonPrimary") : QStringLiteral("actionButton"));
    button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

bool isUsernameDuplicate(const QStringList& names, const QString& candidate)
{
    const QString normalized = candidate.trimmed().toLower();
    for (const QString& name : names) {
        if (name.trimmed().toLower() == normalized) {
            return true;
        }
    }
    return false;
}

void centerDialog(QDialog& dialog, QWidget* parent)
{
    dialog.adjustSize();
    if (parent != nullptr) {
        const QPoint center = parent->mapToGlobal(parent->rect().center());
        dialog.move(center.x() - dialog.width() / 2, center.y() - dialog.height() / 2);
    }
}

void animateDialog(QDialog& dialog)
{
    dialog.setWindowOpacity(0.0);
    QTimer::singleShot(0, &dialog, [&dialog]() {
        auto* fade = new QPropertyAnimation(&dialog, "windowOpacity", &dialog);
        fade->setDuration(170);
        fade->setStartValue(0.0);
        fade->setEndValue(1.0);
        fade->start(QAbstractAnimation::DeleteWhenStopped);
    });
}

QString baseDialogStyle()
{
    return QStringLiteral(
        "#customPopupShell { background:#fffef8; border:2px solid #111; border-radius:2px; }"
        "#customPopupTitle { color:#090909; font:900 15pt 'Trebuchet MS'; letter-spacing:0.5px; }"
        "#customPopupBody { color:#17232d; font:800 10.5pt 'Trebuchet MS'; line-height:120%; }"
        "#customPopupInput { min-height:42px; padding:4px 10px; border:2px solid #111; border-radius:4px; background:white; color:#111; font:900 12pt 'Trebuchet MS'; }"
        "QPushButton { border-radius:4px; font:900 9pt 'Trebuchet MS'; letter-spacing:1px; min-height:40px; }"
        "#primaryPopupButton { background:#111; color:white; border:none; }"
        "#primaryPopupButton:hover { background:#2a2a2a; }"
        "#secondaryPopupButton { background:#e7e2d4; color:#111; border:1px solid #111; }"
        "#secondaryPopupButton:hover { background:#f4efe2; }"
        "#filePopupButton { text-align:left; padding:10px 14px; background:white; color:#111; border:1px solid #111; }"
        "#filePopupButton:hover { background:#eaf6ff; border:2px solid #1262c5; }"
    );
}

void showCustomNotice(QWidget* parent, const QString& titleText, const QString& bodyText, const QString& buttonText = QStringLiteral("OK"))
{
    QDialog dialog(parent, Qt::Dialog | Qt::FramelessWindowHint);
    dialog.setModal(true);
    dialog.setAttribute(Qt::WA_TranslucentBackground);

    auto* root = new QVBoxLayout(&dialog);
    root->setContentsMargins(20, 20, 20, 20);

    auto* shell = new QFrame(&dialog);
    shell->setObjectName(QStringLiteral("customPopupShell"));
    shell->setMinimumSize(430, 230);
    root->addWidget(shell);

    auto* layout = new QVBoxLayout(shell);
    layout->setContentsMargins(24, 18, 24, 18);
    layout->setSpacing(14);

    auto* title = new QLabel(titleText.toUpper(), shell);
    title->setAlignment(Qt::AlignCenter);
    title->setObjectName(QStringLiteral("customPopupTitle"));
    layout->addWidget(title);

    auto* body = new QLabel(bodyText, shell);
    body->setAlignment(Qt::AlignCenter);
    body->setWordWrap(true);
    body->setObjectName(QStringLiteral("customPopupBody"));
    layout->addWidget(body, 1);

    auto* button = new QPushButton(buttonText.toUpper(), shell);
    button->setObjectName(QStringLiteral("primaryPopupButton"));
    button->setCursor(Qt::PointingHandCursor);
    layout->addWidget(button);

    dialog.setStyleSheet(baseDialogStyle());
    QObject::connect(button, &QPushButton::clicked, &dialog, &QDialog::accept);
    centerDialog(dialog, parent);
    animateDialog(dialog);
    dialog.exec();
}

bool showCustomQuestion(QWidget* parent, const QString& titleText, const QString& bodyText, const QString& acceptText, const QString& rejectText)
{
    QDialog dialog(parent, Qt::Dialog | Qt::FramelessWindowHint);
    dialog.setModal(true);
    dialog.setAttribute(Qt::WA_TranslucentBackground);

    auto* root = new QVBoxLayout(&dialog);
    root->setContentsMargins(20, 20, 20, 20);

    auto* shell = new QFrame(&dialog);
    shell->setObjectName(QStringLiteral("customPopupShell"));
    shell->setMinimumSize(430, 230);
    root->addWidget(shell);

    auto* layout = new QVBoxLayout(shell);
    layout->setContentsMargins(24, 18, 24, 18);
    layout->setSpacing(14);

    auto* title = new QLabel(titleText.toUpper(), shell);
    title->setAlignment(Qt::AlignCenter);
    title->setObjectName(QStringLiteral("customPopupTitle"));
    layout->addWidget(title);

    auto* body = new QLabel(bodyText, shell);
    body->setAlignment(Qt::AlignCenter);
    body->setWordWrap(true);
    body->setObjectName(QStringLiteral("customPopupBody"));
    layout->addWidget(body, 1);

    auto* row = new QHBoxLayout();
    row->setSpacing(10);
    layout->addLayout(row);

    auto* rejectButton = new QPushButton(rejectText.toUpper(), shell);
    rejectButton->setObjectName(QStringLiteral("secondaryPopupButton"));
    rejectButton->setCursor(Qt::PointingHandCursor);
    row->addWidget(rejectButton);

    auto* acceptButton = new QPushButton(acceptText.toUpper(), shell);
    acceptButton->setObjectName(QStringLiteral("primaryPopupButton"));
    acceptButton->setCursor(Qt::PointingHandCursor);
    row->addWidget(acceptButton);

    bool accepted = false;
    QObject::connect(rejectButton, &QPushButton::clicked, &dialog, [&]() {
        accepted = false;
        dialog.reject();
    });
    QObject::connect(acceptButton, &QPushButton::clicked, &dialog, [&]() {
        accepted = true;
        dialog.accept();
    });

    dialog.setStyleSheet(baseDialogStyle());
    centerDialog(dialog, parent);
    animateDialog(dialog);
    dialog.exec();
    return accepted;
}

QString promptCustomText(QWidget* parent, const QString& titleText, const QString& bodyText, const QString& defaultValue, bool* accepted)
{
    QDialog dialog(parent, Qt::Dialog | Qt::FramelessWindowHint);
    dialog.setModal(true);
    dialog.setAttribute(Qt::WA_TranslucentBackground);

    auto* root = new QVBoxLayout(&dialog);
    root->setContentsMargins(20, 20, 20, 20);

    auto* shell = new QFrame(&dialog);
    shell->setObjectName(QStringLiteral("customPopupShell"));
    root->addWidget(shell);

    auto* layout = new QVBoxLayout(shell);
    layout->setContentsMargins(24, 18, 24, 18);
    layout->setSpacing(14);

    auto* title = new QLabel(titleText.toUpper(), shell);
    title->setAlignment(Qt::AlignCenter);
    title->setObjectName(QStringLiteral("customPopupTitle"));
    layout->addWidget(title);

    auto* body = new QLabel(bodyText, shell);
    body->setAlignment(Qt::AlignCenter);
    body->setWordWrap(true);
    body->setObjectName(QStringLiteral("customPopupBody"));
    layout->addWidget(body);

    auto* input = new QLineEdit(defaultValue, shell);
    input->setObjectName(QStringLiteral("customPopupInput"));
    input->selectAll();
    layout->addWidget(input);

    auto* row = new QHBoxLayout();
    row->setSpacing(10);
    layout->addLayout(row);

    auto* cancelButton = new QPushButton(QStringLiteral("CANCEL"), shell);
    cancelButton->setObjectName(QStringLiteral("secondaryPopupButton"));
    cancelButton->setCursor(Qt::PointingHandCursor);
    row->addWidget(cancelButton);

    auto* okButton = new QPushButton(QStringLiteral("OK"), shell);
    okButton->setObjectName(QStringLiteral("primaryPopupButton"));
    okButton->setCursor(Qt::PointingHandCursor);
    row->addWidget(okButton);

    QString result = defaultValue;
    *accepted = false;
    QObject::connect(cancelButton, &QPushButton::clicked, &dialog, [&]() {
        *accepted = false;
        dialog.reject();
    });
    QObject::connect(okButton, &QPushButton::clicked, &dialog, [&]() {
        result = input->text().trimmed();
        *accepted = true;
        dialog.accept();
    });

    dialog.setStyleSheet(baseDialogStyle());
    dialog.resize(460, 260);
    centerDialog(dialog, parent);
    animateDialog(dialog);
    dialog.exec();
    return result;
}

int promptCustomInt(QWidget* parent, const QString& titleText, const QString& bodyText, int minValue, int maxValue, int defaultValue, bool* accepted)
{
    QDialog dialog(parent, Qt::Dialog | Qt::FramelessWindowHint);
    dialog.setModal(true);
    dialog.setAttribute(Qt::WA_TranslucentBackground);

    auto* root = new QVBoxLayout(&dialog);
    root->setContentsMargins(20, 20, 20, 20);

    auto* shell = new QFrame(&dialog);
    shell->setObjectName(QStringLiteral("customPopupShell"));
    root->addWidget(shell);

    auto* layout = new QVBoxLayout(shell);
    layout->setContentsMargins(24, 18, 24, 18);
    layout->setSpacing(14);

    auto* title = new QLabel(titleText.toUpper(), shell);
    title->setAlignment(Qt::AlignCenter);
    title->setObjectName(QStringLiteral("customPopupTitle"));
    layout->addWidget(title);

    auto* body = new QLabel(bodyText, shell);
    body->setAlignment(Qt::AlignCenter);
    body->setWordWrap(true);
    body->setObjectName(QStringLiteral("customPopupBody"));
    layout->addWidget(body);

    auto* input = new QSpinBox(shell);
    input->setObjectName(QStringLiteral("customPopupInput"));
    input->setRange(minValue, maxValue);
    input->setValue(qBound(minValue, defaultValue, maxValue));
    layout->addWidget(input);

    auto* row = new QHBoxLayout();
    row->setSpacing(10);
    layout->addLayout(row);

    auto* cancelButton = new QPushButton(QStringLiteral("CANCEL"), shell);
    cancelButton->setObjectName(QStringLiteral("secondaryPopupButton"));
    cancelButton->setCursor(Qt::PointingHandCursor);
    row->addWidget(cancelButton);

    auto* okButton = new QPushButton(QStringLiteral("OK"), shell);
    okButton->setObjectName(QStringLiteral("primaryPopupButton"));
    okButton->setCursor(Qt::PointingHandCursor);
    row->addWidget(okButton);

    int result = input->value();
    *accepted = false;
    QObject::connect(cancelButton, &QPushButton::clicked, &dialog, [&]() {
        *accepted = false;
        dialog.reject();
    });
    QObject::connect(okButton, &QPushButton::clicked, &dialog, [&]() {
        result = input->value();
        *accepted = true;
        dialog.accept();
    });

    dialog.setStyleSheet(baseDialogStyle());
    dialog.resize(440, 260);
    centerDialog(dialog, parent);
    animateDialog(dialog);
    dialog.exec();
    return result;
}

QString promptSaveFilePicker(QWidget* parent, const QString& directory)
{
    QDir saveDir(directory);
    const QStringList files = saveDir.entryList(QStringList() << QStringLiteral("*.txt"), QDir::Files, QDir::Name);
    if (files.isEmpty()) {
        showCustomNotice(parent, QStringLiteral("Load Game"), QStringLiteral("Belum ada save file di folder data."));
        return {};
    }

    QDialog dialog(parent, Qt::Dialog | Qt::FramelessWindowHint);
    dialog.setModal(true);
    dialog.setAttribute(Qt::WA_TranslucentBackground);

    auto* root = new QVBoxLayout(&dialog);
    root->setContentsMargins(20, 20, 20, 20);

    auto* shell = new QFrame(&dialog);
    shell->setObjectName(QStringLiteral("customPopupShell"));
    root->addWidget(shell);

    auto* layout = new QVBoxLayout(shell);
    layout->setContentsMargins(24, 18, 24, 18);
    layout->setSpacing(14);

    auto* title = new QLabel(QStringLiteral("LOAD GAME"), shell);
    title->setAlignment(Qt::AlignCenter);
    title->setObjectName(QStringLiteral("customPopupTitle"));
    layout->addWidget(title);

    auto* scroll = new QScrollArea(shell);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    layout->addWidget(scroll, 1);

    auto* listHost = new QWidget(scroll);
    auto* listLayout = new QVBoxLayout(listHost);
    listLayout->setContentsMargins(0, 0, 0, 0);
    listLayout->setSpacing(8);
    scroll->setWidget(listHost);

    QString selected;
    for (const QString& file : files) {
        auto* fileButton = new QPushButton(file, listHost);
        fileButton->setObjectName(QStringLiteral("filePopupButton"));
        fileButton->setCursor(Qt::PointingHandCursor);
        listLayout->addWidget(fileButton);
        QObject::connect(fileButton, &QPushButton::clicked, &dialog, [&, file]() {
            selected = file;
            dialog.accept();
        });
    }

    auto* cancelButton = new QPushButton(QStringLiteral("CANCEL"), shell);
    cancelButton->setObjectName(QStringLiteral("secondaryPopupButton"));
    cancelButton->setCursor(Qt::PointingHandCursor);
    layout->addWidget(cancelButton);
    QObject::connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    dialog.setStyleSheet(baseDialogStyle());
    dialog.resize(460, 420);
    centerDialog(dialog, parent);
    animateDialog(dialog);
    dialog.exec();
    return selected;
}

}  // namespace

GameWindow::GameWindow(QWidget* parent)
    : QWidget(parent),
      session(this)
{
    setObjectName(QStringLiteral("gameWindow"));
    setMinimumSize(1260, 820);

    accentColorByPlayer.insert(QStringLiteral("Player 1"), QColor(28, 97, 214));
    accentColorByPlayer.insert(QStringLiteral("Player 2"), QColor(216, 40, 40));
    accentColorByPlayer.insert(QStringLiteral("Player 3"), QColor(36, 38, 42));
    accentColorByPlayer.insert(QStringLiteral("Player 4"), QColor(126, 130, 138));

    buildRootPages();
    configureSession();
    configureConnections();

    setStyleSheet(
        "#gameWindow { background: #edf2f7; }"
        "#boardShell {"
        "  background: rgba(255,255,255,0.97);"
        "  border: 1px solid #d7dee6;"
        "  border-radius: 20px;"
        "}"
        "#sidebarPanel {"
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 rgba(170, 210, 228, 0.96), stop:1 rgba(160, 201, 220, 0.98));"
        "  border: 1px solid rgba(123, 168, 188, 0.92);"
        "  border-radius: 22px;"
        "}"
        "#playerHeader {"
        "  background: rgba(255,255,255,0.38);"
        "  border-top-left-radius: 22px;"
        "  border-top-right-radius: 22px;"
        "  border-bottom: 1px solid rgba(111, 148, 165, 0.45);"
        "}"
        "#playerAvatar {"
        "  background: rgba(255,255,255,0.86);"
        "  border: 2px solid rgba(255,255,255,0.96);"
        "  border-radius: 11px;"
        "}"
        "#playerName { color:#10181f; font: 900 15pt 'Trebuchet MS'; }"
        "#playerMoney { color:#2f8c2f; font: 900 18pt 'Trebuchet MS'; }"
        "#portfolioSection, #statsSection, #actionsSection, #historySection { background: transparent; }"
        "#sidebarDivider { background: rgba(111, 150, 167, 0.55); margin: 6px 18px 4px 18px; }"
        "#historyHeaderFrame {"
        "  background: rgba(255,255,255,0.92);"
        "  border: 1px solid rgba(138,160,174,0.95);"
        "  border-top-left-radius: 10px;"
        "  border-top-right-radius: 10px;"
        "}"
        "#historyTitle { color:#243744; font:900 8.7pt 'Trebuchet MS'; letter-spacing:1px; }"
        "#historyFilter { color:#596d7d; font:700 8pt 'Courier New'; }"
        "#historyScroll {"
        "  background: rgba(255,255,255,0.82);"
        "  border-left: 1px solid rgba(138,160,174,0.95);"
        "  border-right: 1px solid rgba(138,160,174,0.95);"
        "  border-bottom: 1px solid rgba(138,160,174,0.95);"
        "  border-bottom-left-radius: 10px;"
        "  border-bottom-right-radius: 10px;"
        "}"
        "#statLabelPrimary, #statLabelSecondary { color:#245987; font:900 10pt 'Trebuchet MS'; }"
        "#statLabelSecondary { color:#3f7a33; }"
        "#playerSwitchButton {"
        "  min-width: 54px;"
        "  min-height: 32px;"
        "  padding: 4px 10px;"
        "  border-radius: 16px;"
        "  border: 1px solid rgba(78,119,136,0.50);"
        "  background: rgba(255,255,255,0.72);"
        "  color: #173142;"
        "  font: 900 8.5pt 'Trebuchet MS';"
        "}"
        "#playerSwitchButton:hover {"
        "  background: rgba(255,255,255,0.94);"
        "  border: 2px solid #347cb7;"
        "}"
        "#actionButtonPrimary, #actionButton {"
        "  padding: 6px 12px;"
        "  border-radius: 10px;"
        "  border: 1px solid rgba(134,152,166,0.9);"
        "  background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #ffffff, stop:1 #dfe7ec);"
        "  color: #162432;"
        "  font: 900 9.5pt 'Trebuchet MS';"
        "}"
        "#actionButtonPrimary {"
        "  background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #ffffff, stop:1 #dbefff);"
        "  border-color: rgba(70, 123, 189, 0.85);"
        "}"
        "#actionButtonPrimary:hover, #actionButton:hover {"
        "  background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #ffffff, stop:1 #eef4f8);"
        "}"
        "#actionButtonPrimary:disabled, #actionButton:disabled {"
        "  color: rgba(74,88,102,0.55);"
        "  background: rgba(231,235,239,0.86);"
        "}"
        "QDialog { background: #d9d4c5; }"
    );

    pageStack->setCurrentWidget(startMenuPage);
}

void GameWindow::buildRootPages()
{
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    pageStack = new QStackedWidget(this);
    rootLayout->addWidget(pageStack);

    startMenuPage = new StartMenuPage(this);
    setupPage = new GameSetupPage(this);
    gamePage = buildGamePage();

    pageStack->addWidget(startMenuPage);
    pageStack->addWidget(setupPage);
    pageStack->addWidget(gamePage);
}

QWidget* GameWindow::buildGamePage()
{
    auto* page = new QWidget(this);

    auto* rootLayout = new QHBoxLayout(page);
    rootLayout->setContentsMargins(18, 18, 18, 18);
    rootLayout->setSpacing(16);

    auto* boardShell = new QFrame(page);
    boardShell->setObjectName(QStringLiteral("boardShell"));
    auto* boardLayout = new QVBoxLayout(boardShell);
    boardLayout->setContentsMargins(18, 18, 18, 18);
    boardLayout->setSpacing(0);

    boardWidget = new BoardWidget(boardShell);
    boardWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    boardLayout->addWidget(boardWidget, 1);
    rootLayout->addWidget(boardShell, 1);

    sidebarPanel = new QFrame(page);
    sidebarPanel->setObjectName(QStringLiteral("sidebarPanel"));
    sidebarPanel->setMinimumWidth(320);
    sidebarPanel->setMaximumWidth(350);
    rootLayout->addWidget(sidebarPanel, 0);

    auto* sidebarLayout = new QVBoxLayout(sidebarPanel);
    sidebarLayout->setContentsMargins(0, 0, 0, 0);
    sidebarLayout->setSpacing(0);

    auto* playerHeader = new QFrame(sidebarPanel);
    playerHeader->setObjectName(QStringLiteral("playerHeader"));
    auto* playerHeaderLayout = new QHBoxLayout(playerHeader);
    playerHeaderLayout->setContentsMargins(18, 16, 18, 14);
    playerHeaderLayout->setSpacing(12);

    playerAvatarLabel = new QLabel(playerHeader);
    playerAvatarLabel->setObjectName(QStringLiteral("playerAvatar"));
    playerAvatarLabel->setAlignment(Qt::AlignCenter);
    playerAvatarLabel->setFixedSize(64, 64);
    playerHeaderLayout->addWidget(playerAvatarLabel, 0, Qt::AlignTop);

    auto* playerTextColumn = new QVBoxLayout();
    playerTextColumn->setSpacing(4);
    playerHeaderLayout->addLayout(playerTextColumn, 1);

    playerNameLabel = new QLabel(QStringLiteral("Player"), playerHeader);
    playerNameLabel->setObjectName(QStringLiteral("playerName"));
    playerTextColumn->addWidget(playerNameLabel);

    playerMoneyLabel = new QLabel(QStringLiteral("M0"), playerHeader);
    playerMoneyLabel->setObjectName(QStringLiteral("playerMoney"));
    playerTextColumn->addWidget(playerMoneyLabel);

    playerSwitchButton = new QToolButton(playerHeader);
    playerSwitchButton->setObjectName(QStringLiteral("playerSwitchButton"));
    playerSwitchButton->setText(QStringLiteral("--"));
    playerSwitchButton->setToolButtonStyle(Qt::ToolButtonTextOnly);
    playerSwitchButton->setPopupMode(QToolButton::InstantPopup);
    playerSwitchButton->setCursor(Qt::PointingHandCursor);
    playerHeaderLayout->addWidget(playerSwitchButton, 0, Qt::AlignTop | Qt::AlignRight);

    sidebarLayout->addWidget(playerHeader, 0);

    auto* portfolioSection = new QFrame(sidebarPanel);
    portfolioSection->setObjectName(QStringLiteral("portfolioSection"));
    auto* portfolioLayout = new QVBoxLayout(portfolioSection);
    portfolioLayout->setContentsMargins(18, 4, 18, 4);
    portfolioLayout->setSpacing(0);

    portfolioWidget = new PropertyPortfolioWidget(portfolioSection);
    portfolioWidget->setFixedHeight(208);
    portfolioLayout->addWidget(portfolioWidget, 1);
    sidebarLayout->addWidget(portfolioSection, 0);

    auto* divider = new QFrame(sidebarPanel);
    divider->setObjectName(QStringLiteral("sidebarDivider"));
    divider->setFixedHeight(1);
    sidebarLayout->addWidget(divider, 0);

    auto* statsSection = new QFrame(sidebarPanel);
    statsSection->setObjectName(QStringLiteral("statsSection"));
    auto* statsLayout = new QHBoxLayout(statsSection);
    statsLayout->setContentsMargins(18, 8, 18, 10);
    statsLayout->setSpacing(10);

    auto* houseTitle = new QLabel(QStringLiteral("Houses"), statsSection);
    houseTitle->setStyleSheet(QStringLiteral("color:#244150;font:800 8.5pt 'Trebuchet MS';"));
    statsLayout->addWidget(houseTitle);

    houseCountLabel = new QLabel(QStringLiteral("0"), statsSection);
    houseCountLabel->setObjectName(QStringLiteral("statLabelPrimary"));
    statsLayout->addWidget(houseCountLabel);

    auto* hotelTitle = new QLabel(QStringLiteral("Hotels"), statsSection);
    hotelTitle->setStyleSheet(QStringLiteral("color:#244150;font:800 8.5pt 'Trebuchet MS';margin-left:8px;"));
    statsLayout->addWidget(hotelTitle);

    hotelCountLabel = new QLabel(QStringLiteral("0"), statsSection);
    hotelCountLabel->setObjectName(QStringLiteral("statLabelSecondary"));
    statsLayout->addWidget(hotelCountLabel);

    statsLayout->addStretch(1);
    sidebarLayout->addWidget(statsSection, 0);

    auto* actionsSection = new QFrame(sidebarPanel);
    actionsSection->setObjectName(QStringLiteral("actionsSection"));
    auto* actionsLayout = new QGridLayout(actionsSection);
    actionsLayout->setContentsMargins(18, 0, 18, 12);
    actionsLayout->setHorizontalSpacing(10);
    actionsLayout->setVerticalSpacing(10);

    rollButton = new QToolButton(actionsSection);
    setDiceButton = new QToolButton(actionsSection);
    useSkillButton = new QToolButton(actionsSection);
    payFineButton = new QToolButton(actionsSection);
    buildButton = new QToolButton(actionsSection);
    mortgageButton = new QToolButton(actionsSection);
    redeemButton = new QToolButton(actionsSection);
    saveButton = new QToolButton(actionsSection);

    configureActionButton(rollButton, QStringLiteral("ROLL DICE"), style()->standardIcon(QStyle::SP_MediaPlay), true);
    configureActionButton(setDiceButton, QStringLiteral("SET DICE"), style()->standardIcon(QStyle::SP_BrowserReload));
    configureActionButton(useSkillButton, QStringLiteral("USE SKILL"), style()->standardIcon(QStyle::SP_CommandLink));
    configureActionButton(payFineButton, QStringLiteral("PAY FINE"), style()->standardIcon(QStyle::SP_MessageBoxWarning));
    configureActionButton(buildButton, QStringLiteral("BUILD"), style()->standardIcon(QStyle::SP_FileDialogNewFolder));
    configureActionButton(mortgageButton, QStringLiteral("MORTGAGE"), style()->standardIcon(QStyle::SP_DialogApplyButton));
    configureActionButton(redeemButton, QStringLiteral("UNMORTGAGE"), style()->standardIcon(QStyle::SP_DialogResetButton));
    configureActionButton(saveButton, QStringLiteral("SAVE"), style()->standardIcon(QStyle::SP_DialogSaveButton));

    actionsLayout->addWidget(rollButton, 0, 0);
    actionsLayout->addWidget(setDiceButton, 0, 1);
    actionsLayout->addWidget(useSkillButton, 1, 0);
    actionsLayout->addWidget(payFineButton, 1, 1);
    actionsLayout->addWidget(buildButton, 2, 0);
    actionsLayout->addWidget(mortgageButton, 2, 1);
    actionsLayout->addWidget(redeemButton, 3, 0);
    actionsLayout->addWidget(saveButton, 3, 1);
    sidebarLayout->addWidget(actionsSection, 0);

    auto* historySection = new QFrame(sidebarPanel);
    historySection->setObjectName(QStringLiteral("historySection"));
    auto* historySectionLayout = new QVBoxLayout(historySection);
    historySectionLayout->setContentsMargins(18, 0, 18, 18);
    historySectionLayout->setSpacing(0);

    historyHeaderFrame = new QFrame(historySection);
    historyHeaderFrame->setObjectName(QStringLiteral("historyHeaderFrame"));
    auto* historyHeaderLayout = new QHBoxLayout(historyHeaderFrame);
    historyHeaderLayout->setContentsMargins(10, 8, 10, 8);
    historyHeaderLayout->setSpacing(6);

    auto* historyTitle = new QLabel(QStringLiteral("HISTORY"), historyHeaderFrame);
    historyTitle->setObjectName(QStringLiteral("historyTitle"));
    historyHeaderLayout->addWidget(historyTitle);
    historyHeaderLayout->addStretch(1);

    auto* historyFilter = new QLabel(QStringLiteral("LIVE"), historyHeaderFrame);
    historyFilter->setObjectName(QStringLiteral("historyFilter"));
    historyHeaderLayout->addWidget(historyFilter);
    historySectionLayout->addWidget(historyHeaderFrame, 0);

    auto* historyScroll = new QScrollArea(historySection);
    historyScroll->setObjectName(QStringLiteral("historyScroll"));
    historyScroll->setWidgetResizable(true);
    historyScroll->setFrameShape(QFrame::NoFrame);
    historyScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    historyScroll->setMinimumHeight(220);

    auto* historyContent = new QWidget(historyScroll);
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
    auto* dialogLayout = new QVBoxLayout(propertyCardDialog);
    dialogLayout->setContentsMargins(12, 12, 12, 12);
    propertyCardWidget = new PropertyCardWidget(propertyCardDialog);
    dialogLayout->addWidget(propertyCardWidget);

    return page;
}

void GameWindow::configureConnections()
{
    connect(startMenuPage, &StartMenuPage::newGameRequested, this, [this]() {
        setupPage->resetForm();
        pageStack->setCurrentWidget(setupPage);
    });

    connect(startMenuPage, &StartMenuPage::loadGameRequested, this, [this]() {
        loadGameFromPicker();
    });

    connect(startMenuPage, &StartMenuPage::settingsRequested, this, [this]() {
        showCustomNotice(this, QStringLiteral("Settings"), QStringLiteral("Menu settings belum diimplementasikan pada versi ini."));
    });

    connect(startMenuPage, &StartMenuPage::leaderboardRequested, this, [this]() {
        showCustomNotice(this, QStringLiteral("Leaderboard"), QStringLiteral("Leaderboard belum tersedia pada versi ini."));
    });

    connect(setupPage, &GameSetupPage::backRequested, this, [this]() {
        pageStack->setCurrentWidget(startMenuPage);
    });

    connect(setupPage, &GameSetupPage::startRequested, this, [this]() {
        startNewGame();
    });

    connect(boardWidget, &BoardWidget::propertySelected, this, [this](int propertyId) {
        const PropertyViewState* propertyState = propertyStateForId(propertyId);
        if (propertyState != nullptr && !propertyState->ownerUsername.isEmpty() &&
            propertyState->ownerUsername != QStringLiteral("BANK")) {
            setSelectedPlayer(propertyState->ownerUsername);
        }
        setSelectedProperty(propertyId, true);
    });

    connect(portfolioWidget, &PropertyPortfolioWidget::propertyActivated, this, [this](int propertyId) {
        setSelectedProperty(propertyId, true);
    });

    connect(rollButton, &QToolButton::clicked, this, [this]() {
        executeSessionAction(
            QStringLiteral("Dadu berhasil dilempar."),
            [this](QString* errorMessage) { return session.rollDice(errorMessage); }
        );
    });

    connect(setDiceButton, &QToolButton::clicked, this, [this]() {
        handleManualRollRequested();
    });

    connect(useSkillButton, &QToolButton::clicked, this, [this]() {
        executeSessionAction(
            QStringLiteral("Kartu kemampuan diproses."),
            [this](QString* errorMessage) { return session.useSkill(errorMessage); }
        );
    });

    connect(payFineButton, &QToolButton::clicked, this, [this]() {
        executeSessionAction(
            QStringLiteral("Pembayaran denda diproses."),
            [this](QString* errorMessage) { return session.payJailFine(errorMessage); }
        );
    });

    connect(buildButton, &QToolButton::clicked, this, [this]() {
        executeSessionAction(
            QStringLiteral("Pembangunan berhasil diproses."),
            [this](QString* errorMessage) { return session.build(errorMessage); }
        );
    });

    connect(mortgageButton, &QToolButton::clicked, this, [this]() {
        executeSessionAction(
            QStringLiteral("Properti berhasil digadaikan."),
            [this](QString* errorMessage) { return session.mortgage(errorMessage); }
        );
    });

    connect(redeemButton, &QToolButton::clicked, this, [this]() {
        executeSessionAction(
            QStringLiteral("Properti berhasil di-unmortgage."),
            [this](QString* errorMessage) { return session.redeem(errorMessage); }
        );
    });

    connect(saveButton, &QToolButton::clicked, this, [this]() {
        saveCurrentGame();
    });
}

void GameWindow::configureSession()
{
    session.setMovementStepHandler([this](const Player&, int) {
        refreshViewModels();
        syncSelectedPlayer();
        syncSelectedProperty();
        refreshBoardPawns();
        refreshPlayerHeader();
        QApplication::processEvents(QEventLoop::AllEvents);
    });

    session.setPropertyPurchaseHandler([this](const Player& player, const PropertyTile& property) {
        return promptPropertyPurchase(player, property);
    });

    session.setPropertyNoticeHandler([this](const Player& player, const PropertyTile& property) {
        showPropertyNotice(player, property);
    });

    session.setBoardTileSelectionHandler([this](const QString& title, const QVector<int>& validTileIndices, bool allowCancel) {
        return promptBoardTileSelection(title, validTileIndices, allowCancel);
    });

    session.setTurnChangedHandler([this]() {
        refreshViewModels();
        selectedPlayerUsername = activePlayerUsername;
        syncSelectedProperty();
        refreshBoardPawns();
        refreshSidebar();
        refreshActionAvailability();
        QApplication::processEvents(QEventLoop::AllEvents);
    });

    QString errorMessage;
    if (session.loadConfig(&errorMessage)) {
        properties = session.getConfigData().getPropertyConfigs();
        propertyCardWidget->setConfigData(session.getConfigData());
    }
}

void GameWindow::startNewGame()
{
    const QStringList names = setupPage->playerNames();
    if (names.size() < 2 || names.size() > 4) {
        showCustomNotice(this, QStringLiteral("Validasi"), QStringLiteral("Jumlah pemain harus 2 sampai 4."));
        return;
    }

    QStringList seen;
    std::vector<std::string> playerNames;
    playerNames.reserve(names.size());

    for (const QString& rawName : names) {
        const QString name = rawName.trimmed();
        if (name.isEmpty()) {
            showCustomNotice(this, QStringLiteral("Validasi"), QStringLiteral("Semua nama pemain yang aktif harus diisi."));
            return;
        }
        if (name.size() < 3) {
            showCustomNotice(this, QStringLiteral("Validasi"), QStringLiteral("Username minimal 3 karakter."));
            return;
        }
        if (name.contains(QRegularExpression(QStringLiteral("\\s")))) {
            showCustomNotice(this, QStringLiteral("Validasi"), QStringLiteral("Username tidak boleh mengandung spasi."));
            return;
        }
        if (isUsernameDuplicate(seen, name)) {
            showCustomNotice(this, QStringLiteral("Validasi"), QStringLiteral("Username pemain harus unik."));
            return;
        }

        seen.append(name);
        playerNames.push_back(name.toStdString());
    }

    QString errorMessage;
    if (!session.startNewGame(playerNames, &errorMessage)) {
        if (!errorMessage.isEmpty()) {
            showCustomNotice(this, QStringLiteral("Game Baru"), errorMessage);
        }
        return;
    }

    properties = session.getConfigData().getPropertyConfigs();
    propertyCardWidget->setConfigData(session.getConfigData());
    finishDialogShown = false;
    selectedPropertyId = 0;
    selectedPlayerUsername.clear();

    pageStack->setCurrentWidget(gamePage);
    refreshScene(QStringLiteral("Permainan baru berhasil dimulai."));
}

void GameWindow::loadGameFromPicker()
{
    const QString directory = saveDirectoryPath();
    const QString filename = promptSaveFilePicker(this, directory);

    if (filename.isEmpty()) {
        return;
    }

    QString errorMessage;
    if (!session.loadGame(filename.toStdString(), &errorMessage)) {
        if (!errorMessage.isEmpty()) {
            showCustomNotice(this, QStringLiteral("Load Game"), errorMessage);
        }
        return;
    }

    properties = session.getConfigData().getPropertyConfigs();
    propertyCardWidget->setConfigData(session.getConfigData());
    finishDialogShown = false;
    selectedPropertyId = 0;
    selectedPlayerUsername.clear();

    pageStack->setCurrentWidget(gamePage);
    refreshScene(QStringLiteral("Permainan berhasil dimuat."));
}

void GameWindow::saveCurrentGame()
{
    if (!session.isReady()) {
        return;
    }

    bool accepted = false;
    QString filename = promptCustomText(
        this,
        QStringLiteral("Simpan Game"),
        QStringLiteral("Masukkan nama file save:"),
        QStringLiteral("game_sesi1.txt"),
        &accepted
    ).trimmed();

    if (!accepted || filename.isEmpty()) {
        return;
    }

    if (!filename.endsWith(QStringLiteral(".txt"), Qt::CaseInsensitive)) {
        filename += QStringLiteral(".txt");
    }

    SaveManager saveManager;
    const QString resolvedPath = QString::fromStdString(saveManager.getResolvedDataPath(filename.toStdString()));
    if (QFileInfo::exists(resolvedPath)) {
        const bool overwrite = showCustomQuestion(
            this,
            QStringLiteral("Timpa File"),
            QStringLiteral("File \"%1\" sudah ada. Timpa file lama?").arg(QFileInfo(resolvedPath).fileName()),
            QStringLiteral("Yes"),
            QStringLiteral("No")
        );
        if (!overwrite) {
            return;
        }
    }

    executeSessionAction(
        QStringLiteral("Permainan berhasil disimpan ke %1.").arg(QFileInfo(resolvedPath).fileName()),
        [this, filename](QString* errorMessage) {
            return session.saveGame(filename.toStdString(), errorMessage);
        }
    );
}

void GameWindow::handleManualRollRequested()
{
    bool accepted = false;
    const int dieOne = promptCustomInt(
        this,
        QStringLiteral("Set Dadu"),
        QStringLiteral("Nilai dadu pertama (1-6):"),
        1,
        6,
        1,
        &accepted
    );
    if (!accepted) {
        return;
    }

    const int dieTwo = promptCustomInt(
        this,
        QStringLiteral("Set Dadu"),
        QStringLiteral("Nilai dadu kedua (1-6):"),
        1,
        6,
        1,
        &accepted
    );
    if (!accepted) {
        return;
    }

    executeSessionAction(
        QStringLiteral("Dadu manual berhasil dijalankan."),
        [this, dieOne, dieTwo](QString* errorMessage) {
            return session.setDiceAndRoll(dieOne, dieTwo, errorMessage);
        }
    );
}

void GameWindow::executeSessionAction(const QString& successFallback, const std::function<bool(QString*)>& action)
{
    QString errorMessage;
    const bool success = action(&errorMessage);
    if (!success && !errorMessage.isEmpty()) {
        lastStatusText = errorMessage;
    }

    applyPendingMessages(success ? successFallback : errorMessage);
    refreshScene(lastStatusText);
    showGameFinishedDialogIfNeeded();
}

void GameWindow::showGameFinishedDialogIfNeeded()
{
    if (!session.isGameEnded() || finishDialogShown) {
        return;
    }

    finishDialogShown = true;
    const std::vector<Player*> winners = session.determineWinner();

    QStringList lines;
    lines.append(QStringLiteral("Permainan selesai pada turn %1.").arg(session.getCurrentTurn()));
    if (winners.empty()) {
        lines.append(QStringLiteral("Tidak ada pemenang yang dapat ditentukan."));
    } else {
        lines.append(QStringLiteral("Pemenang:"));
        for (Player* winner : winners) {
            if (winner != nullptr) {
                lines.append(QStringLiteral("- %1 (Total Wealth: M%2)")
                    .arg(QString::fromStdString(winner->getUsername()))
                    .arg(winner->getTotalWealth()));
            }
        }
    }

    showCustomNotice(this, QStringLiteral("Game Selesai"), lines.join('\n'), QStringLiteral("CONTINUE"));
    refreshActionAvailability();
}

void GameWindow::refreshViewModels()
{
    playerOverviews.clear();
    propertyStateById.clear();
    historyEntries.clear();
    activePlayerUsername.clear();

    if (!session.isReady()) {
        return;
    }

    const GameState gameState = session.snapshot();
    activePlayerUsername = QString::fromStdString(gameState.getActivePlayerUsername());

    int playerIndex = 0;
    for (const Player& player : session.getPlayers()) {
        const QString username = QString::fromStdString(player.getUsername());
        if (!accentColorByPlayer.contains(username)) {
            accentColorByPlayer.insert(username, playerAccent(playerIndex));
        }

        playerOverviews.append({
            username,
            pawnAssetNameForPlayer(username),
            accentColorForPlayer(username),
            player.getBalance(),
            player.getPosition(),
            static_cast<int>(player.getHand().size()),
            static_cast<int>(player.getProperties().size()),
            username == activePlayerUsername,
            player.isJailed(),
            player.hasRolledThisTurn(),
            player.hasUsedSkillThisTurn(),
            player.hasTakenActionThisTurn()
        });
        ++playerIndex;
    }

    for (const PropertyState& propertyState : gameState.getPropertyStates()) {
        for (const PropertyConfig& property : properties) {
            if (property.getCode() != propertyState.getCode()) {
                continue;
            }

            PropertyViewState viewState;
            viewState.ownerUsername = QString::fromStdString(propertyState.getOwnerUsername());
            viewState.mortgaged = propertyState.getStatus() == PropertyStatus::MORTGAGED;
            viewState.buildingLevel = propertyState.getBuildingLevel() == "H"
                ? 5
                : QString::fromStdString(propertyState.getBuildingLevel()).toInt();
            propertyStateById.insert(property.getId(), viewState);
            break;
        }
    }

    const std::vector<LogEntry>& logs = session.getLogger().getAll();
    historyEntries.reserve(static_cast<int>(logs.size()));
    for (const LogEntry& log : logs) {
        historyEntries.append({
            log.getTurn(),
            QString::fromStdString(log.getUsername()),
            QString::fromStdString(log.getActionType()),
            QString::fromStdString(log.getDetail())
        });
    }
}

void GameWindow::refreshScene(const QString& statusText)
{
    if (!statusText.isEmpty()) {
        lastStatusText = statusText;
    }

    refreshViewModels();
    syncSelectedPlayer();
    syncSelectedProperty();
    refreshBoardPawns();
    refreshSidebar();
    refreshActionAvailability();
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
        playerAvatarLabel->setPixmap(QPixmap());
        playerAvatarLabel->setText(QStringLiteral("--"));
        playerSwitchButton->setText(QStringLiteral("--"));
        return;
    }

    playerNameLabel->setText(player->name);
    playerMoneyLabel->setText(MonopolyUi::formatCurrency(player->balance));
    playerSwitchButton->setText(shortPlayerLabel(player->name));

    playerAvatarLabel->setPixmap(QPixmap());
    playerAvatarLabel->setText(shortPlayerLabel(player->name));
    const QColor accent = player->accentColor.isValid() ? player->accentColor : QColor(90, 190, 240);
    playerAvatarLabel->setStyleSheet(QStringLiteral(
        "background: %1;"
        "border: 2px solid rgba(255,255,255,0.95);"
        "border-radius: 11px;"
        "color: white;"
        "font: 900 15pt 'Trebuchet MS';"
    ).arg(accent.name()));
}

void GameWindow::refreshPlayerSelector()
{
    auto* menu = new QMenu(playerSwitchButton);
    menu->setStyleSheet(QStringLiteral(
        "QMenu {"
        "  background: rgba(255,255,255,0.98);"
        "  border: 1px solid rgba(123, 153, 171, 0.85);"
        "  border-radius: 10px;"
        "  padding: 6px;"
        "}"
        "QMenu::item {"
        "  padding: 8px 18px;"
        "  color: #173142;"
        "  font: 800 9pt 'Trebuchet MS';"
        "}"
        "QMenu::item:selected {"
        "  background: rgba(52, 124, 183, 0.16);"
        "  border-radius: 7px;"
        "}"
    ));

    for (const PlayerOverview& player : playerOverviews) {
        QAction* action = menu->addAction(QStringLiteral("%1  %2")
            .arg(shortPlayerLabel(player.name), player.name));
        action->setData(player.name);
        action->setCheckable(true);
        action->setChecked(player.name == selectedPlayerUsername);
        connect(action, &QAction::triggered, this, [this, action]() {
            setSelectedPlayer(action->data().toString());
        });
    }

    QMenu* oldMenu = playerSwitchButton->menu();
    playerSwitchButton->setMenu(menu);
    if (oldMenu != nullptr) {
        oldMenu->deleteLater();
    }
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
    while (QLayoutItem* item = historyEntriesLayout->takeAt(0)) {
        if (item->widget() != nullptr) {
            item->widget()->deleteLater();
        }
        delete item;
    }

    if (historyEntries.isEmpty()) {
        auto* emptyLabel = new QLabel(QStringLiteral("Belum ada transaksi yang tercatat."), sidebarPanel);
        emptyLabel->setWordWrap(true);
        emptyLabel->setStyleSheet(QStringLiteral("color:#324b58;font:600 8pt 'Trebuchet MS';padding:12px 10px;"));
        historyEntriesLayout->addWidget(emptyLabel);
        historyEntriesLayout->addStretch(1);
        return;
    }

    for (auto it = historyEntries.crbegin(); it != historyEntries.crend(); ++it) {
        auto* entryFrame = new QFrame(sidebarPanel);
        entryFrame->setStyleSheet(QStringLiteral(
            "background: rgba(255,255,255,0.78);"
            "border-left: 4px solid %1;"
            "border-bottom: 1px solid rgba(211, 221, 226, 0.95);"
        ).arg(historyAccentColor(*it)));

        auto* entryLayout = new QVBoxLayout(entryFrame);
        entryLayout->setContentsMargins(10, 8, 10, 8);
        entryLayout->setSpacing(3);

        auto* metaLabel = new QLabel(
            QStringLiteral("Turn %1 | %2 | %3")
                .arg(it->turn)
                .arg(it->username)
                .arg(it->actionType),
            entryFrame
        );
        metaLabel->setStyleSheet(QStringLiteral("color:#8294a1;font:700 7pt 'Trebuchet MS';"));
        entryLayout->addWidget(metaLabel);

        auto* detailLabel = new QLabel(it->detail, entryFrame);
        detailLabel->setWordWrap(true);
        detailLabel->setStyleSheet(QStringLiteral("color:#17232d;font:700 8.5pt 'Trebuchet MS';"));
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

    QVector<BoardWidget::BuildingData> buildingData;
    buildingData.reserve(propertyStateById.size());
    for (auto it = propertyStateById.constBegin(); it != propertyStateById.constEnd(); ++it) {
        if (it.value().buildingLevel <= 0) {
            continue;
        }
        buildingData.append({it.key() - 1, it.value().buildingLevel});
    }
    boardWidget->setBuildings(buildingData);
}

void GameWindow::refreshActionAvailability()
{
    const PlayerOverview* currentPlayerOverview = playerOverviewByUsername(activePlayerUsername);
    const Player* currentPlayer = session.getCurrentPlayer();
    const bool hasGame = session.isReady() && currentPlayerOverview != nullptr && currentPlayer != nullptr && !session.isGameEnded();
    const bool canRoll = hasGame && !currentPlayerOverview->hasRolledThisTurn &&
        !(currentPlayerOverview->isInJail && currentPlayer->getJailTurns() > 3);

    rollButton->setEnabled(canRoll);
    setDiceButton->setEnabled(canRoll);
    useSkillButton->setEnabled(hasGame && !currentPlayerOverview->hasUsedSkillThisTurn &&
        !currentPlayerOverview->hasRolledThisTurn && !currentPlayer->getHand().empty());
    payFineButton->setEnabled(hasGame && currentPlayerOverview->isInJail);
    buildButton->setEnabled(hasGame);
    mortgageButton->setEnabled(hasGame);
    redeemButton->setEnabled(hasGame);
    saveButton->setEnabled(hasGame && !currentPlayerOverview->hasTakenActionThisTurn);
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

void GameWindow::syncSelectedProperty()
{
    if (propertyConfigForId(selectedPropertyId) != nullptr) {
        return;
    }

    const Player* currentPlayer = session.getCurrentPlayer();
    if (currentPlayer != nullptr) {
        const int propertyId = currentPlayer->getPosition() + 1;
        if (propertyConfigForId(propertyId) != nullptr) {
            selectedPropertyId = propertyId;
            return;
        }
    }

    if (!properties.empty()) {
        selectedPropertyId = properties.front().getId();
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
    if (propertyConfigForId(propertyId) == nullptr) {
        return;
    }

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

void GameWindow::showPropertyNotice(const Player& player, const PropertyTile& property)
{
    const int propertyId = property.getIndex() + 1;
    if (propertyConfigForId(propertyId) == nullptr) {
        return;
    }

    selectedPlayerUsername = QString::fromStdString(player.getUsername());
    setSelectedProperty(propertyId, false);
    refreshScene(QStringLiteral("%1 mendarat di %2.")
        .arg(QString::fromStdString(player.getUsername()))
        .arg(QString::fromStdString(property.getName())));

    QDialog dialog(this, Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    dialog.setWindowTitle(QStringLiteral("Property Notice"));
    dialog.setModal(true);
    dialog.resize(430, 690);

    auto* layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(14, 14, 14, 14);
    layout->setSpacing(12);

    auto* card = new PropertyCardWidget(&dialog);
    card->setConfigData(session.getConfigData());
    card->setSelectedProperty(propertyId);
    layout->addWidget(card, 1);

    auto* info = new QLabel(
        QStringLiteral("%1 mendapatkan %2.")
            .arg(QString::fromStdString(player.getUsername()))
            .arg(MonopolyUi::singleLineTileName(property.getName())),
        &dialog
    );
    info->setWordWrap(true);
    info->setAlignment(Qt::AlignCenter);
    info->setStyleSheet(QStringLiteral("color:#17232d;font:800 10pt 'Trebuchet MS';"));
    layout->addWidget(info);

    auto* okButton = new QPushButton(QStringLiteral("OK"), &dialog);
    okButton->setMinimumHeight(52);
    okButton->setCursor(Qt::PointingHandCursor);
    layout->addWidget(okButton);

    dialog.setStyleSheet(QStringLiteral(
        "QDialog { background:#e3dece; }"
        "QPushButton {"
        "  border-radius: 10px;"
        "  padding: 10px 18px;"
        "  font: 900 11pt 'Trebuchet MS';"
        "  letter-spacing: 1px;"
        "  background:#1159c7;"
        "  color:white;"
        "  border:1px solid #0d49a4;"
        "}"
        "QPushButton:hover { background:#1d6ee6; }"
    ));

    connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    dialog.exec();
}

int GameWindow::promptBoardTileSelection(const QString& title, const QVector<int>& validTileIndices, bool allowCancel)
{
    if (validTileIndices.isEmpty()) {
        return -1;
    }

    QSet<int> selectable;
    for (int index : validTileIndices) {
        if (index >= 0 && index < 40) {
            selectable.insert(index);
        }
    }

    if (selectable.isEmpty()) {
        return -1;
    }

    int selectedTileIndex = -1;
    QEventLoop loop;
    const QMetaObject::Connection connection = connect(
        boardWidget,
        &BoardWidget::tileSelected,
        &loop,
        [&](int tileIndex) {
            selectedTileIndex = tileIndex;
            loop.quit();
        }
    );
    const QMetaObject::Connection cancelConnection = connect(
        boardWidget,
        &BoardWidget::tileSelectionCanceled,
        &loop,
        [&]() {
            selectedTileIndex = -1;
            loop.quit();
        }
    );

    boardWidget->setTileSelectionMode(selectable, title, allowCancel);
    refreshScene(title);
    QApplication::processEvents(QEventLoop::AllEvents);

    loop.exec();

    disconnect(connection);
    disconnect(cancelConnection);
    boardWidget->clearTileSelectionMode();
    refreshScene(lastStatusText);
    return selectedTileIndex;
}

bool GameWindow::promptPropertyPurchase(const Player& player, const PropertyTile& property)
{
    const int propertyId = property.getIndex() + 1;
    if (propertyConfigForId(propertyId) == nullptr) {
        return player.canAfford(property.getBuyPrice());
    }

    selectedPlayerUsername = QString::fromStdString(player.getUsername());
    setSelectedProperty(propertyId, false);
    refreshScene(QStringLiteral("%1 mendarat di %2. Pilih Pay untuk membeli atau Auction untuk melelang.")
        .arg(QString::fromStdString(player.getUsername()))
        .arg(QString::fromStdString(property.getName())));

    QDialog dialog(this, Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    dialog.setWindowTitle(QStringLiteral("Property Available"));
    dialog.setModal(true);
    dialog.resize(430, 720);

    auto* layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(14, 14, 14, 14);
    layout->setSpacing(12);

    auto* card = new PropertyCardWidget(&dialog);
    card->setConfigData(session.getConfigData());
    card->setSelectedProperty(propertyId);
    layout->addWidget(card, 1);

    auto* info = new QLabel(
        QStringLiteral("%1 dapat membeli %2 seharga %3.")
            .arg(QString::fromStdString(player.getUsername()))
            .arg(MonopolyUi::singleLineTileName(property.getName()))
            .arg(MonopolyUi::formatCurrency(property.getBuyPrice())),
        &dialog
    );
    info->setWordWrap(true);
    info->setAlignment(Qt::AlignCenter);
    info->setStyleSheet(QStringLiteral("color:#17232d;font:800 10pt 'Trebuchet MS';"));
    layout->addWidget(info);

    auto* buttonRow = new QHBoxLayout();
    buttonRow->setSpacing(12);
    layout->addLayout(buttonRow);

    auto* payButton = new QPushButton(QStringLiteral("PAY"), &dialog);
    payButton->setMinimumHeight(52);
    payButton->setCursor(Qt::PointingHandCursor);
    payButton->setEnabled(player.canAfford(property.getBuyPrice()));

    auto* auctionButton = new QPushButton(QStringLiteral("AUCTION"), &dialog);
    auctionButton->setMinimumHeight(52);
    auctionButton->setCursor(Qt::PointingHandCursor);

    buttonRow->addWidget(payButton);
    buttonRow->addWidget(auctionButton);

    dialog.setStyleSheet(QStringLiteral(
        "QDialog { background:#e3dece; }"
        "QPushButton {"
        "  border-radius: 10px;"
        "  padding: 10px 18px;"
        "  font: 900 11pt 'Trebuchet MS';"
        "  letter-spacing: 1px;"
        "}"
        "QPushButton:enabled {"
        "  background:#1159c7;"
        "  color:white;"
        "  border:1px solid #0d49a4;"
        "}"
        "QPushButton:enabled:hover { background:#1d6ee6; }"
        "QPushButton:disabled {"
        "  background:#d8dde3;"
        "  color:#8d99a6;"
        "  border:1px solid #c3cad2;"
        "}"
    ));

    bool shouldBuy = false;
    connect(payButton, &QPushButton::clicked, &dialog, [&]() {
        shouldBuy = true;
        dialog.accept();
    });
    connect(auctionButton, &QPushButton::clicked, &dialog, [&]() {
        shouldBuy = false;
        dialog.reject();
    });

    dialog.exec();
    return shouldBuy;
}

void GameWindow::applyPendingMessages(const QString& fallback)
{
    const QStringList messages = session.takePendingMessages();
    if (!messages.isEmpty()) {
        lastStatusText = messages.mid(qMax(0, messages.size() - 3)).join('\n');
    } else if (!fallback.isEmpty()) {
        lastStatusText = fallback;
    }
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

const PropertyViewState* GameWindow::propertyStateForId(int propertyId) const
{
    const auto it = propertyStateById.constFind(propertyId);
    if (it == propertyStateById.constEnd()) {
        return nullptr;
    }
    return &it.value();
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

        const PropertyViewState* state = propertyStateForId(property.getId());
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
    Q_UNUSED(username);
    return {};
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

QString GameWindow::currentTurnStatusText() const
{
    const PlayerOverview* selectedPlayer = playerOverviewByUsername(selectedPlayerUsername);
    const PlayerOverview* currentPlayer = playerOverviewByUsername(activePlayerUsername);
    if (selectedPlayer == nullptr || currentPlayer == nullptr) {
        return lastStatusText;
    }

    const int maxTurn = session.getMaxTurn();
    const QString turnText = maxTurn > 0
        ? QStringLiteral("Turn %1 / %2").arg(session.getCurrentTurn()).arg(maxTurn)
        : QStringLiteral("Turn %1 / Unlimited").arg(session.getCurrentTurn());

    QString summary = QStringLiteral("%1 | Active: %2 | Cards: %3 | Properties: %4")
        .arg(turnText)
        .arg(currentPlayer->name)
        .arg(selectedPlayer->handCount)
        .arg(selectedPlayer->propertyCount);

    if (selectedPlayer->isInJail) {
        summary += QStringLiteral("\nStatus: JAILED");
    }

    if (!lastStatusText.isEmpty()) {
        summary += QStringLiteral("\n") + lastStatusText;
    }

    return summary;
}

QString GameWindow::saveDirectoryPath() const
{
    SaveManager saveManager;
    const QString resolved = QString::fromStdString(saveManager.getResolvedDataPath("placeholder.txt"));
    return QFileInfo(resolved).absolutePath();
}
