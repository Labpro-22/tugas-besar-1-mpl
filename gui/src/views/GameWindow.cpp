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
#include <QFont>
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
#include <QResizeEvent>
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
    button->setMinimumHeight(56);
    button->setObjectName(primary ? QStringLiteral("actionButtonPrimary") : QStringLiteral("actionButton"));
    button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
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
    resize(1240, 800);
    setMinimumSize(960, 640);

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
        "  padding: 4px 8px;"
        "  border-radius: 10px;"
        "  border: 1px solid rgba(134,152,166,0.9);"
        "  background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #ffffff, stop:1 #dfe7ec);"
        "  color: #162432;"
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

    gamePageLayout = new QHBoxLayout(page);
    gamePageLayout->setContentsMargins(18, 18, 18, 18);
    gamePageLayout->setSpacing(16);

    boardShell = new QFrame(page);
    boardShell->setObjectName(QStringLiteral("boardShell"));
    boardShellLayout = new QVBoxLayout(boardShell);
    boardShellLayout->setContentsMargins(18, 18, 18, 18);
    boardShellLayout->setSpacing(0);

    boardWidget = new BoardWidget(boardShell);
    boardWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    boardShellLayout->addWidget(boardWidget, 1);
    gamePageLayout->addWidget(boardShell, 1);

    sidebarPanel = new QFrame(page);
    sidebarPanel->setObjectName(QStringLiteral("sidebarPanel"));
    sidebarPanel->setMinimumWidth(320);
    sidebarPanel->setMaximumWidth(350);
    sidebarPanel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    gamePageLayout->addWidget(sidebarPanel, 0);

    sidebarLayout = new QVBoxLayout(sidebarPanel);
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
    actionsLayout = new QGridLayout(actionsSection);
    actionsLayout->setContentsMargins(18, 0, 18, 12);
    actionsLayout->setHorizontalSpacing(10);
    actionsLayout->setVerticalSpacing(10);
    actionsLayout->setColumnStretch(0, 1);
    actionsLayout->setColumnStretch(1, 1);

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

    historyScroll = new QScrollArea(historySection);
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

    updateResponsiveLayout();
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
    session.setLiquidationPlanHandler([this](
        const Player& player,
        int targetAmount,
        const std::vector<LiquidationCandidate>& candidates,
        std::vector<LiquidationDecision>& decisions
    ) {
        return promptLiquidationPlan(player, targetAmount, candidates, decisions);
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
    const std::vector<Player>& allPlayers = session.getPlayers();

    int activePlayers = 0;
    QStringList remainingPlayers;
    for (const Player& player : allPlayers) {
        if (!player.isBankrupt()) {
            ++activePlayers;
            remainingPlayers.append(QString::fromStdString(player.getUsername()));
        }
    }

    const bool maxTurnReached = session.getMaxTurn() > 0 &&
        session.getCurrentTurn() >= session.getMaxTurn() &&
        activePlayers > 1;

    QDialog dialog(this, Qt::Dialog | Qt::FramelessWindowHint);
    dialog.setModal(true);
    dialog.setAttribute(Qt::WA_TranslucentBackground);

    auto* root = new QVBoxLayout(&dialog);
    root->setContentsMargins(20, 20, 20, 20);

    auto* shell = new QFrame(&dialog);
    shell->setObjectName(QStringLiteral("finishShell"));
    root->addWidget(shell);

    auto* layout = new QVBoxLayout(shell);
    layout->setContentsMargins(24, 20, 24, 20);
    layout->setSpacing(14);

    auto* title = new QLabel(QStringLiteral("GAME FINISHED"), shell);
    title->setAlignment(Qt::AlignCenter);
    title->setObjectName(QStringLiteral("finishTitle"));
    layout->addWidget(title);

    auto* reason = new QLabel(
        maxTurnReached
            ? QStringLiteral("Permainan selesai! Batas giliran tercapai.")
            : QStringLiteral("Permainan selesai! Semua pemain kecuali satu bangkrut."),
        shell
    );
    reason->setAlignment(Qt::AlignCenter);
    reason->setWordWrap(true);
    reason->setObjectName(QStringLiteral("finishReason"));
    layout->addWidget(reason);

    auto* turnInfo = new QLabel(
        session.getMaxTurn() > 0
            ? QStringLiteral("Turn %1 / %2").arg(session.getCurrentTurn()).arg(session.getMaxTurn())
            : QStringLiteral("Turn %1").arg(session.getCurrentTurn()),
        shell
    );
    turnInfo->setAlignment(Qt::AlignCenter);
    turnInfo->setObjectName(QStringLiteral("finishTurnInfo"));
    layout->addWidget(turnInfo);

    auto* scroll = new QScrollArea(shell);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setMinimumHeight(maxTurnReached ? 260 : 170);
    layout->addWidget(scroll);

    auto* content = new QWidget(scroll);
    auto* contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(10);

    if (maxTurnReached) {
        auto* recapTitle = new QLabel(QStringLiteral("REKAP PEMAIN"), content);
        recapTitle->setObjectName(QStringLiteral("finishSectionTitle"));
        contentLayout->addWidget(recapTitle);

        for (const Player& player : allPlayers) {
            auto* card = new QFrame(content);
            card->setObjectName(QStringLiteral("finishPlayerCard"));
            auto* cardLayout = new QVBoxLayout(card);
            cardLayout->setContentsMargins(14, 12, 14, 12);
            cardLayout->setSpacing(4);

            auto* nameLabel = new QLabel(QString::fromStdString(player.getUsername()), card);
            nameLabel->setObjectName(QStringLiteral("finishPlayerName"));
            cardLayout->addWidget(nameLabel);

            QStringList stats;
            stats.append(QStringLiteral("Uang      : %1").arg(MonopolyUi::formatCurrency(player.getBalance())));
            stats.append(QStringLiteral("Properti  : %1").arg(player.getProperties().size()));
            stats.append(QStringLiteral("Kartu     : %1").arg(player.getHand().size()));
            if (player.isBankrupt()) {
                stats.append(QStringLiteral("Status    : BANGKRUT"));
            }

            auto* statLabel = new QLabel(stats.join('\n'), card);
            statLabel->setWordWrap(true);
            statLabel->setObjectName(QStringLiteral("finishPlayerStats"));
            cardLayout->addWidget(statLabel);

            contentLayout->addWidget(card);
        }
    } else {
        auto* remainingTitle = new QLabel(QStringLiteral("PEMAIN TERSISA"), content);
        remainingTitle->setObjectName(QStringLiteral("finishSectionTitle"));
        contentLayout->addWidget(remainingTitle);

        auto* remainingLabel = new QLabel(
            remainingPlayers.isEmpty()
                ? QStringLiteral("- Tidak ada")
                : QStringLiteral("- %1").arg(remainingPlayers.join(QStringLiteral("\n- "))),
            content
        );
        remainingLabel->setWordWrap(true);
        remainingLabel->setObjectName(QStringLiteral("finishRemaining"));
        contentLayout->addWidget(remainingLabel);
    }

    auto* winnerTitle = new QLabel(QStringLiteral("PEMENANG"), content);
    winnerTitle->setObjectName(QStringLiteral("finishSectionTitle"));
    contentLayout->addWidget(winnerTitle);

    QStringList winnerNames;
    for (Player* winner : winners) {
        if (winner != nullptr) {
            winnerNames.append(QString::fromStdString(winner->getUsername()));
        }
    }

    auto* winnerCard = new QFrame(content);
    winnerCard->setObjectName(QStringLiteral("finishWinnerCard"));
    auto* winnerLayout = new QVBoxLayout(winnerCard);
    winnerLayout->setContentsMargins(16, 14, 16, 14);
    winnerLayout->setSpacing(6);

    auto* winnerName = new QLabel(
        winnerNames.isEmpty()
            ? QStringLiteral("Tidak ada pemenang yang dapat ditentukan.")
            : winnerNames.join(QStringLiteral(", ")),
        winnerCard
    );
    winnerName->setWordWrap(true);
    winnerName->setAlignment(Qt::AlignCenter);
    winnerName->setObjectName(QStringLiteral("finishWinnerName"));
    winnerLayout->addWidget(winnerName);

    if (maxTurnReached && !winners.empty()) {
        QStringList tiebreakInfo;
        for (Player* winner : winners) {
            if (winner == nullptr) {
                continue;
            }
            tiebreakInfo.append(QStringLiteral(
                "%1 | Uang: %2 | Properti: %3 | Kartu: %4")
                .arg(QString::fromStdString(winner->getUsername()))
                .arg(MonopolyUi::formatCurrency(winner->getBalance()))
                .arg(winner->getProperties().size())
                .arg(winner->getHand().size()));
        }

        auto* winnerInfo = new QLabel(tiebreakInfo.join('\n'), winnerCard);
        winnerInfo->setWordWrap(true);
        winnerInfo->setAlignment(Qt::AlignCenter);
        winnerInfo->setObjectName(QStringLiteral("finishWinnerInfo"));
        winnerLayout->addWidget(winnerInfo);
    }

    contentLayout->addWidget(winnerCard);
    contentLayout->addStretch(1);
    scroll->setWidget(content);

    auto* continueButton = new QPushButton(QStringLiteral("CONTINUE"), shell);
    continueButton->setObjectName(QStringLiteral("finishPrimaryButton"));
    continueButton->setCursor(Qt::PointingHandCursor);
    continueButton->setMinimumHeight(44);
    layout->addWidget(continueButton);

    dialog.setStyleSheet(QStringLiteral(
        "#finishShell { background:#fffef8; border:2px solid #111; border-radius:2px; }"
        "#finishTitle { color:#090909; font:900 17pt 'Trebuchet MS'; letter-spacing:0.5px; }"
        "#finishReason { color:#17232d; font:800 11pt 'Trebuchet MS'; }"
        "#finishTurnInfo { color:#45606f; font:800 9pt 'Trebuchet MS'; }"
        "#finishSectionTitle { color:#111; font:900 9pt 'Trebuchet MS'; letter-spacing:1px; }"
        "#finishPlayerCard { background:white; border:1px solid #111; }"
        "#finishPlayerName { color:#111; font:900 11pt 'Trebuchet MS'; }"
        "#finishPlayerStats { color:#253744; font:800 9.2pt 'Trebuchet MS'; }"
        "#finishRemaining { background:white; border:1px solid #111; color:#253744; font:800 9.5pt 'Trebuchet MS'; padding:14px; }"
        "#finishWinnerCard { background:#111; border:1px solid #111; }"
        "#finishWinnerName { color:white; font:900 13pt 'Trebuchet MS'; }"
        "#finishWinnerInfo { color:#d7e6f1; font:800 8.8pt 'Trebuchet MS'; }"
        "#finishPrimaryButton { background:#111; color:white; border:none; border-radius:4px; font:900 9pt 'Trebuchet MS'; letter-spacing:1px; }"
        "#finishPrimaryButton:hover { background:#2a2a2a; }"
    ));

    connect(continueButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    dialog.resize(520, maxTurnReached ? 700 : 500);
    centerDialog(dialog, this);
    animateDialog(dialog);
    dialog.exec();
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
    refreshSelectedPropertyDetails();
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
    const int avatarRadius = qMax(10, playerAvatarLabel->width() / 6);
    const int avatarFontSize = std::clamp(playerAvatarLabel->width() / 4, 12, 18);
    playerAvatarLabel->setStyleSheet(QStringLiteral(
        "background: %1;"
        "border: 2px solid rgba(255,255,255,0.95);"
        "border-radius: %2px;"
        "color: white;"
        "font: 900 %3pt 'Trebuchet MS';"
    ).arg(accent.name()).arg(avatarRadius).arg(avatarFontSize));
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

    QVector<BoardWidget::OwnerData> ownerData;
    ownerData.reserve(propertyStateById.size());
    for (auto it = propertyStateById.constBegin(); it != propertyStateById.constEnd(); ++it) {
        const QString ownerUsername = it.value().ownerUsername;
        if (ownerUsername.isEmpty() || ownerUsername == QStringLiteral("BANK")) {
            continue;
        }
        ownerData.append({
            it.key() - 1,
            ownerUsername,
            accentColorForPlayer(ownerUsername),
            it.value().mortgaged
        });
    }
    boardWidget->setOwners(ownerData);
}

void GameWindow::refreshSelectedPropertyDetails()
{
    if (propertyCardWidget == nullptr || propertyConfigForId(selectedPropertyId) == nullptr) {
        return;
    }

    const PropertyViewState* state = propertyStateForId(selectedPropertyId);
    const QString ownerUsername = state == nullptr || state->ownerUsername.isEmpty()
        ? QStringLiteral("BANK")
        : state->ownerUsername;
    propertyCardWidget->setOwnershipInfo(
        ownerUsername,
        ownerUsername == QStringLiteral("BANK") ? QColor(70, 78, 88) : accentColorForPlayer(ownerUsername),
        state != nullptr && state->mortgaged,
        state == nullptr ? 0 : state->buildingLevel
    );
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
        !currentPlayerOverview->hasRolledThisTurn &&
        !currentPlayerOverview->hasTakenActionThisTurn &&
        !currentPlayer->getHand().empty());
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
    refreshSelectedPropertyDetails();

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
    const PropertyViewState* state = propertyStateForId(propertyId);
    const QString ownerUsername = state == nullptr || state->ownerUsername.isEmpty()
        ? QStringLiteral("BANK")
        : state->ownerUsername;
    card->setOwnershipInfo(
        ownerUsername,
        ownerUsername == QStringLiteral("BANK") ? QColor(70, 78, 88) : accentColorForPlayer(ownerUsername),
        state != nullptr && state->mortgaged,
        state == nullptr ? 0 : state->buildingLevel
    );
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

bool GameWindow::promptLiquidationPlan(
    const Player& player,
    int targetAmount,
    const std::vector<LiquidationCandidate>& candidates,
    std::vector<LiquidationDecision>& decisions
)
{
    decisions.clear();
    if (candidates.empty()) {
        return false;
    }

    selectedPlayerUsername = QString::fromStdString(player.getUsername());
    refreshScene(QStringLiteral("%1 harus melakukan likuidasi aset.").arg(QString::fromStdString(player.getUsername())));

    QDialog dialog(this, Qt::Dialog | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    dialog.setAttribute(Qt::WA_TranslucentBackground);
    dialog.setModal(false);

    auto* root = new QVBoxLayout(&dialog);
    root->setContentsMargins(20, 20, 20, 20);

    auto* shell = new QFrame(&dialog);
    shell->setObjectName(QStringLiteral("liquidationShell"));
    root->addWidget(shell);

    auto* layout = new QVBoxLayout(shell);
    layout->setContentsMargins(24, 18, 24, 18);
    layout->setSpacing(12);

    auto* title = new QLabel(QStringLiteral("ASSET LIQUIDATION"), shell);
    title->setAlignment(Qt::AlignCenter);
    title->setObjectName(QStringLiteral("liquidationTitle"));
    layout->addWidget(title);

    auto* subtitle = new QLabel(
        QStringLiteral("%1 harus menutup kewajiban sebesar %2.")
            .arg(QString::fromStdString(player.getUsername()))
            .arg(MonopolyUi::formatCurrency(targetAmount)),
        shell
    );
    subtitle->setAlignment(Qt::AlignCenter);
    subtitle->setWordWrap(true);
    subtitle->setObjectName(QStringLiteral("liquidationBody"));
    layout->addWidget(subtitle);

    auto* balanceLabel = new QLabel(shell);
    balanceLabel->setAlignment(Qt::AlignCenter);
    balanceLabel->setObjectName(QStringLiteral("liquidationMeta"));
    layout->addWidget(balanceLabel);

    auto* plannedLabel = new QLabel(shell);
    plannedLabel->setAlignment(Qt::AlignCenter);
    plannedLabel->setObjectName(QStringLiteral("liquidationMeta"));
    layout->addWidget(plannedLabel);

    auto* shortageLabel = new QLabel(shell);
    shortageLabel->setAlignment(Qt::AlignCenter);
    shortageLabel->setObjectName(QStringLiteral("liquidationShortage"));
    layout->addWidget(shortageLabel);

    auto* hintLabel = new QLabel(QStringLiteral("Pilih properti langsung dari board. Rencana baru akan dieksekusi saat FINALIZE."), shell);
    hintLabel->setAlignment(Qt::AlignCenter);
    hintLabel->setWordWrap(true);
    hintLabel->setObjectName(QStringLiteral("liquidationHint"));
    layout->addWidget(hintLabel);

    auto* scroll = new QScrollArea(shell);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setMinimumHeight(170);
    layout->addWidget(scroll);

    auto* listHost = new QWidget(scroll);
    auto* listLayout = new QVBoxLayout(listHost);
    listLayout->setContentsMargins(0, 0, 0, 0);
    listLayout->setSpacing(10);
    scroll->setWidget(listHost);

    auto* planListLabel = new QLabel(listHost);
    planListLabel->setWordWrap(true);
    planListLabel->setTextFormat(Qt::RichText);
    planListLabel->setObjectName(QStringLiteral("liquidationPlanList"));
    listLayout->addWidget(planListLabel);
    listLayout->addStretch(1);
    scroll->setWidget(listHost);

    auto* actionRow = new QHBoxLayout();
    actionRow->setSpacing(10);
    layout->addLayout(actionRow);

    auto* sellButton = new QPushButton(QStringLiteral("SELL TO BANK"), shell);
    sellButton->setObjectName(QStringLiteral("liquidationSecondaryButton"));
    sellButton->setCursor(Qt::PointingHandCursor);
    sellButton->setMinimumHeight(44);
    actionRow->addWidget(sellButton);

    auto* mortgageButton = new QPushButton(QStringLiteral("MORTGAGE"), shell);
    mortgageButton->setObjectName(QStringLiteral("liquidationSecondaryButton"));
    mortgageButton->setCursor(Qt::PointingHandCursor);
    mortgageButton->setMinimumHeight(44);
    actionRow->addWidget(mortgageButton);

    auto* footerRow = new QHBoxLayout();
    footerRow->setSpacing(10);
    layout->addLayout(footerRow);

    auto* cancelButton = new QPushButton(QStringLiteral("CANCEL ALL"), shell);
    cancelButton->setObjectName(QStringLiteral("liquidationDangerButton"));
    cancelButton->setCursor(Qt::PointingHandCursor);
    cancelButton->setMinimumHeight(46);
    footerRow->addWidget(cancelButton);

    auto* undoButton = new QPushButton(QStringLiteral("UNDO"), shell);
    undoButton->setObjectName(QStringLiteral("liquidationSecondaryButton"));
    undoButton->setCursor(Qt::PointingHandCursor);
    undoButton->setMinimumHeight(46);
    footerRow->addWidget(undoButton);

    auto* finalizeButton = new QPushButton(QStringLiteral("FINALIZE"), shell);
    finalizeButton->setObjectName(QStringLiteral("liquidationPrimaryButton"));
    finalizeButton->setCursor(Qt::PointingHandCursor);
    finalizeButton->setMinimumHeight(46);
    footerRow->addWidget(finalizeButton);

    dialog.setStyleSheet(QStringLiteral(
        "#liquidationShell {"
        "  background:#fffef8;"
        "  border:2px solid #111;"
        "  border-radius:2px;"
        "}"
        "#liquidationTitle { color:#090909; font:900 16pt 'Trebuchet MS'; letter-spacing:0.5px; }"
        "#liquidationBody { color:#17232d; font:800 10.5pt 'Trebuchet MS'; }"
        "#liquidationMeta { color:#2d4757; font:800 9.5pt 'Trebuchet MS'; }"
        "#liquidationShortage { font:900 13pt 'Trebuchet MS'; }"
        "#liquidationHint { color:#5d6670; font:700 8.8pt 'Trebuchet MS'; }"
        "#liquidationPlanList { color:#17232d; font:800 9.2pt 'Trebuchet MS'; background:white; border:1px solid #111; padding:12px; }"
        "QPushButton { border-radius:4px; font:900 9pt 'Trebuchet MS'; letter-spacing:1px; }"
        "#liquidationPrimaryButton { background:#111; color:white; border:none; }"
        "#liquidationPrimaryButton:hover { background:#2a2a2a; }"
        "#liquidationPrimaryButton:disabled { background:#c9c5b9; color:#777; }"
        "#liquidationSecondaryButton { background:#e7e2d4; color:#111; border:1px solid #111; }"
        "#liquidationSecondaryButton:hover { background:#f4efe2; }"
        "#liquidationSecondaryButton:disabled { background:#ebe7dc; color:#8a8176; border:1px solid #c7bfb0; }"
        "#liquidationDangerButton { background:#7e1f24; color:white; border:none; }"
        "#liquidationDangerButton:hover { background:#94272c; }"
    ));

    QMap<int, LiquidationActionKind> plannedActions;
    QVector<int> plannedOrder;
    bool accepted = false;

    auto candidateForTile = [&](int tileIndex) -> const LiquidationCandidate* {
        for (const LiquidationCandidate& candidate : candidates) {
            if (candidate.tileIndex == tileIndex) {
                return &candidate;
            }
        }
        return nullptr;
    };

    auto plannedGain = [&]() -> int {
        int total = 0;
        for (int tileIndex : plannedOrder) {
            const LiquidationCandidate* candidate = candidateForTile(tileIndex);
            if (candidate == nullptr) {
                continue;
            }
            total += plannedActions.value(tileIndex) == LiquidationActionKind::Sell
                ? candidate->sellValue
                : candidate->mortgageValue;
        }
        return total;
    };

    auto eligibleTiles = [&](LiquidationActionKind action) -> QVector<int> {
        QVector<int> result;
        for (const LiquidationCandidate& candidate : candidates) {
            if (plannedActions.contains(candidate.tileIndex)) {
                continue;
            }

            const int value = action == LiquidationActionKind::Sell
                ? candidate.sellValue
                : candidate.mortgageValue;
            if (value > 0) {
                result.append(candidate.tileIndex);
            }
        }
        return result;
    };

    auto refreshPlanner = [&]() {
        const int gain = plannedGain();
        const int shortage = targetAmount - (player.getBalance() + gain);

        balanceLabel->setText(QStringLiteral("Saldo sekarang: %1").arg(MonopolyUi::formatCurrency(player.getBalance())));
        plannedLabel->setText(QStringLiteral("Total dana rencana likuidasi: %1").arg(MonopolyUi::formatCurrency(gain)));

        if (shortage > 0) {
            shortageLabel->setText(QStringLiteral("SISA KEKURANGAN: -%1").arg(MonopolyUi::formatCurrency(shortage)));
            shortageLabel->setStyleSheet(QStringLiteral("color:#c62828;font:900 13pt 'Trebuchet MS';"));
        } else {
            shortageLabel->setText(QStringLiteral("TARGET TERPENUHI: +%1").arg(MonopolyUi::formatCurrency(-shortage)));
            shortageLabel->setStyleSheet(QStringLiteral("color:#2f8c2f;font:900 13pt 'Trebuchet MS';"));
        }

        QStringList lines;
        if (plannedOrder.isEmpty()) {
            lines.append(QStringLiteral("<b>Belum ada aset yang direncanakan.</b><br><span style='color:#6a737c;'>Pilih SELL TO BANK atau MORTGAGE lalu klik tile di board.</span>"));
        } else {
            for (int tileIndex : plannedOrder) {
                const LiquidationCandidate* candidate = candidateForTile(tileIndex);
                if (candidate == nullptr) {
                    continue;
                }

                const bool isSell = plannedActions.value(tileIndex) == LiquidationActionKind::Sell;
                const int value = isSell ? candidate->sellValue : candidate->mortgageValue;
                lines.append(QStringLiteral(
                    "<b>%1</b><br><span style='color:#475866;'>%2 (%3)</span><br><span style='color:#1262c5;'>%4</span>")
                    .arg(isSell ? QStringLiteral("JUAL KE BANK") : QStringLiteral("MORTGAGE"))
                    .arg(QString::fromStdString(candidate->name))
                    .arg(QString::fromStdString(candidate->code))
                    .arg(MonopolyUi::formatCurrency(value)));
            }
        }
        planListLabel->setText(lines.join(QStringLiteral("<br><br>")));

        sellButton->setEnabled(!eligibleTiles(LiquidationActionKind::Sell).isEmpty());
        mortgageButton->setEnabled(!eligibleTiles(LiquidationActionKind::Mortgage).isEmpty());
        undoButton->setEnabled(!plannedOrder.isEmpty());
        finalizeButton->setEnabled(shortage <= 0 && !plannedOrder.isEmpty());
    };

    auto setPlannerButtonsEnabled = [&](bool enabled) {
        sellButton->setEnabled(enabled && !eligibleTiles(LiquidationActionKind::Sell).isEmpty());
        mortgageButton->setEnabled(enabled && !eligibleTiles(LiquidationActionKind::Mortgage).isEmpty());
        cancelButton->setEnabled(enabled);
        undoButton->setEnabled(enabled && !plannedOrder.isEmpty());
        finalizeButton->setEnabled(enabled && (targetAmount - (player.getBalance() + plannedGain()) <= 0) && !plannedOrder.isEmpty());
    };

    auto queueSelection = [&](LiquidationActionKind action, const QString& titleText) {
        QVector<int> selectable = eligibleTiles(action);
        if (selectable.isEmpty()) {
            return;
        }

        setPlannerButtonsEnabled(false);
        dialog.hide();
        const int selectedTileIndex = promptBoardTileSelection(titleText, selectable, true);
        dialog.show();
        setPlannerButtonsEnabled(true);
        dialog.raise();
        dialog.activateWindow();

        if (selectedTileIndex < 0 || plannedActions.contains(selectedTileIndex)) {
            refreshPlanner();
            return;
        }

        plannedActions.insert(selectedTileIndex, action);
        plannedOrder.append(selectedTileIndex);
        const LiquidationCandidate* candidate = candidateForTile(selectedTileIndex);
        if (candidate != nullptr) {
            setSelectedProperty(selectedTileIndex + 1, false);
        }
        refreshPlanner();
    };

    connect(sellButton, &QPushButton::clicked, &dialog, [&]() {
        queueSelection(LiquidationActionKind::Sell, QStringLiteral("Pilih properti yang ingin dijual ke Bank."));
    });
    connect(mortgageButton, &QPushButton::clicked, &dialog, [&]() {
        queueSelection(LiquidationActionKind::Mortgage, QStringLiteral("Pilih properti yang ingin digadaikan."));
    });

    QEventLoop loop;
    connect(cancelButton, &QPushButton::clicked, &dialog, [&]() {
        decisions.clear();
        plannedActions.clear();
        plannedOrder.clear();
        refreshPlanner();
    });
    connect(undoButton, &QPushButton::clicked, &dialog, [&]() {
        if (plannedOrder.isEmpty()) {
            return;
        }

        setPlannerButtonsEnabled(false);
        dialog.hide();
        const int selectedTileIndex = promptBoardTileSelection(
            QStringLiteral("Pilih aset dalam rencana yang ingin dibatalkan."),
            plannedOrder,
            true
        );
        dialog.show();
        setPlannerButtonsEnabled(true);
        dialog.raise();
        dialog.activateWindow();

        if (selectedTileIndex >= 0 && plannedActions.contains(selectedTileIndex)) {
            plannedActions.remove(selectedTileIndex);
            plannedOrder.removeOne(selectedTileIndex);
        }
        refreshPlanner();
    });
    connect(finalizeButton, &QPushButton::clicked, &dialog, [&]() {
        if (targetAmount - (player.getBalance() + plannedGain()) > 0 || plannedOrder.isEmpty()) {
            return;
        }

        decisions.clear();
        for (int tileIndex : plannedOrder) {
            decisions.push_back({tileIndex, plannedActions.value(tileIndex)});
        }
        accepted = true;
        dialog.close();
        loop.quit();
    });

    refreshPlanner();
    dialog.resize(500, 560);
    centerDialog(dialog, this);
    animateDialog(dialog);
    dialog.show();
    dialog.raise();
    dialog.activateWindow();
    loop.exec();

    refreshScene(lastStatusText);
    return accepted;
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
    card->setOwnershipInfo(QStringLiteral("BANK"), QColor(70, 78, 88), false, 0);
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

void GameWindow::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    updateResponsiveLayout();
}

void GameWindow::updateResponsiveLayout()
{
    const int windowWidth = qMax(width(), minimumWidth());
    const int windowHeight = qMax(height(), minimumHeight());
    const int shellMargin = std::clamp(windowWidth / 70, 14, 28);
    const int shellSpacing = std::clamp(windowWidth / 90, 12, 22);
    const int sidebarWidth = std::clamp(static_cast<int>(windowWidth * 0.29), 320, 460);
    const int avatarSize = std::clamp(windowHeight / 10, 56, 82);
    const bool compactSidebar = windowHeight < 820;
    const int portfolioMinHeight = std::clamp(windowHeight / (compactSidebar ? 6 : 4), compactSidebar ? 112 : 170, compactSidebar ? 170 : 260);
    const int portfolioMaxHeight = std::clamp(windowHeight / (compactSidebar ? 5 : 3), compactSidebar ? 132 : 210, compactSidebar ? 198 : 320);
    const int historyMinHeight = std::clamp(windowHeight / (compactSidebar ? 7 : 4), compactSidebar ? 88 : 180, compactSidebar ? 150 : 290);
    const int actionHeight = std::clamp(windowHeight / (compactSidebar ? 17 : 12), compactSidebar ? 40 : 54, compactSidebar ? 52 : 78);
    const int actionIcon = std::clamp(actionHeight / 3, compactSidebar ? 16 : 20, compactSidebar ? 20 : 30);
    const int actionFont = std::clamp(actionHeight / 6, compactSidebar ? 7 : 9, compactSidebar ? 9 : 12);
    const int playerNameFont = std::clamp(windowWidth / 80, 11, 18);
    const int moneyFont = std::clamp(windowWidth / 64, 13, 22);
    const int switchFont = std::clamp(windowWidth / 110, 8, 11);

    if (gamePageLayout != nullptr) {
        gamePageLayout->setContentsMargins(shellMargin, shellMargin, shellMargin, shellMargin);
        gamePageLayout->setSpacing(shellSpacing);
    }

    if (boardShellLayout != nullptr) {
        boardShellLayout->setContentsMargins(shellMargin, shellMargin, shellMargin, shellMargin);
    }

    if (sidebarPanel != nullptr) {
        sidebarPanel->setMinimumWidth(sidebarWidth);
        sidebarPanel->setMaximumWidth(sidebarWidth + std::clamp(windowWidth / 80, 18, 28));
    }

    if (sidebarLayout != nullptr) {
        sidebarLayout->setSpacing(0);
    }

    if (playerAvatarLabel != nullptr) {
        playerAvatarLabel->setFixedSize(avatarSize, avatarSize);
    }

    if (playerNameLabel != nullptr) {
        QFont playerNameFontStyle(QStringLiteral("Trebuchet MS"), playerNameFont, QFont::Black);
        playerNameLabel->setFont(playerNameFontStyle);
    }

    if (playerMoneyLabel != nullptr) {
        QFont moneyFontStyle(QStringLiteral("Trebuchet MS"), moneyFont, QFont::Black);
        playerMoneyLabel->setFont(moneyFontStyle);
    }

    if (playerSwitchButton != nullptr) {
        QFont switchButtonFont(QStringLiteral("Trebuchet MS"), switchFont, QFont::Black);
        playerSwitchButton->setFont(switchButtonFont);
        playerSwitchButton->setMinimumSize(std::clamp(sidebarWidth / 5, 60, 86), std::clamp(windowHeight / 22, 34, 46));
    }

    if (portfolioWidget != nullptr) {
        portfolioWidget->setMinimumHeight(portfolioMinHeight);
        portfolioWidget->setMaximumHeight(portfolioMaxHeight);
    }

    if (historyScroll != nullptr) {
        historyScroll->setMinimumHeight(historyMinHeight);
    }

    if (actionsLayout != nullptr) {
        const int horizontalGap = std::clamp(sidebarWidth / 28, 10, 20);
        const int verticalGap = std::clamp(windowHeight / (compactSidebar ? 110 : 70), compactSidebar ? 6 : 9, compactSidebar ? 10 : 16);
        const int actionsPadding = std::clamp(sidebarWidth / 24, compactSidebar ? 10 : 14, compactSidebar ? 18 : 24);
        actionsLayout->setContentsMargins(actionsPadding, 0, actionsPadding, std::clamp(actionsPadding - 4, compactSidebar ? 6 : 10, compactSidebar ? 12 : 20));
        actionsLayout->setHorizontalSpacing(horizontalGap);
        actionsLayout->setVerticalSpacing(verticalGap);

        if (QWidget* actionsSection = actionsLayout->parentWidget()) {
            const int actionsRowsHeight = (actionHeight * 4) + (verticalGap * 3) + actionsLayout->contentsMargins().bottom();
            actionsSection->setMinimumHeight(actionsRowsHeight);
            actionsSection->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
        }
    }

    const QList<QToolButton*> actionButtons = {
        rollButton, setDiceButton, useSkillButton, payFineButton,
        buildButton, mortgageButton, redeemButton, saveButton
    };
    for (QToolButton* button : actionButtons) {
        if (button == nullptr) {
            continue;
        }
        button->setMinimumHeight(actionHeight);
        button->setMaximumHeight(QWIDGETSIZE_MAX);
        button->setIconSize(QSize(actionIcon, actionIcon));
        QFont font(QStringLiteral("Trebuchet MS"), actionFont, QFont::Black);
        button->setFont(font);
    }

    refreshPlayerHeader();
}
