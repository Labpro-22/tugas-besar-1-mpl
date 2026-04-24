#include "core/QtGameIO.hpp"

#include <limits>
#include <stdexcept>
#include <utility>

#include <QButtonGroup>
#include <QDialog>
#include <QEventLoop>
#include <QFrame>
#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPixmap>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QRandomGenerator>
#include <QScrollArea>
#include <QString>
#include <QSpinBox>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

#include "core/DeckFactory.hpp"
#include "models/Enums.hpp"
#include "models/Player.hpp"
#include "models/cards/ActionCard.hpp"
#include "models/cards/SkillCard.hpp"
#include "models/tiles/PropertyTile.hpp"
#include "utils/TransactionLogger.hpp"

namespace {

bool isTestMode()
{
    return qEnvironmentVariableIsSet("NIMONSPOLI_TEST_MODE");
}

QString toQString(const std::string& value)
{
    return QString::fromStdString(value);
}

void logErrorIfPossible(
    TransactionLogger* logger,
    int turn,
    const std::string& username,
    const std::string& detail
)
{
    if (logger != nullptr) {
        logger->log(turn, username, "ERROR", detail);
    }
}

QPixmap makeDiceFace(int value, const QSize& size)
{
    QPixmap pixmap(size);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    const QRectF diceRect(3, 3, size.width() - 6, size.height() - 6);
    painter.setPen(QPen(QColor(15, 18, 22), 2.2));
    painter.setBrush(Qt::white);
    painter.drawRoundedRect(diceRect, 8, 8);

    const qreal dotRadius = qMax<qreal>(4.0, size.width() * 0.075);
    const qreal left = diceRect.left() + diceRect.width() * 0.24;
    const qreal center = diceRect.center().x();
    const qreal right = diceRect.right() - diceRect.width() * 0.24;
    const qreal top = diceRect.top() + diceRect.height() * 0.24;
    const qreal middle = diceRect.center().y();
    const qreal bottom = diceRect.bottom() - diceRect.height() * 0.24;

    auto drawDot = [&](qreal x, qreal y) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(Qt::black);
        painter.drawEllipse(QPointF(x, y), dotRadius, dotRadius);
    };

    switch (value) {
    case 1:
        drawDot(center, middle);
        break;
    case 2:
        drawDot(left, top);
        drawDot(right, bottom);
        break;
    case 3:
        drawDot(left, top);
        drawDot(center, middle);
        drawDot(right, bottom);
        break;
    case 4:
        drawDot(left, top);
        drawDot(right, top);
        drawDot(left, bottom);
        drawDot(right, bottom);
        break;
    case 5:
        drawDot(left, top);
        drawDot(right, top);
        drawDot(center, middle);
        drawDot(left, bottom);
        drawDot(right, bottom);
        break;
    default:
        drawDot(left, top);
        drawDot(right, top);
        drawDot(left, middle);
        drawDot(right, middle);
        drawDot(left, bottom);
        drawDot(right, bottom);
        break;
    }

    return pixmap;
}

QPixmap makeDicePair(int die1, int die2)
{
    const QSize faceSize(92, 92);
    QPixmap pair(faceSize.width() * 2 + 18, faceSize.height());
    pair.fill(Qt::transparent);

    QPainter painter(&pair);
    painter.drawPixmap(0, 0, makeDiceFace(die1, faceSize));
    painter.drawPixmap(faceSize.width() + 18, 0, makeDiceFace(die2, faceSize));
    return pair;
}

QString actionCardTitle(CardType cardType)
{
    return cardType == CardType::CHANCE
        ? QStringLiteral("KESEMPATAN")
        : QStringLiteral("DANA UMUM");
}

QString skillCardTitle(const SkillCard* card)
{
    if (card == nullptr) {
        return QStringLiteral("SKILL");
    }

    QString title = QString::fromStdString(card->getTypeName());
    title.remove(QStringLiteral("Card"), Qt::CaseInsensitive);
    return title.toUpper();
}

void animateDialogEntrance(QDialog& dialog)
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

void centerDialogOnParent(QDialog& dialog, QWidget* parent)
{
    dialog.adjustSize();
    if (parent != nullptr) {
        const QPoint center = parent->mapToGlobal(parent->rect().center());
        dialog.move(center.x() - dialog.width() / 2, center.y() - dialog.height() / 2);
    }
}

void showNoticeCard(QWidget* parent, const QString& titleText, const QString& bodyText, const QString& buttonText)
{
    QDialog dialog(parent, Qt::Dialog | Qt::FramelessWindowHint);
    dialog.setModal(true);
    dialog.setAttribute(Qt::WA_TranslucentBackground);

    auto* root = new QVBoxLayout(&dialog);
    root->setContentsMargins(20, 20, 20, 20);

    auto* shell = new QFrame(&dialog);
    shell->setObjectName(QStringLiteral("noticeCardShell"));
    shell->setMinimumSize(430, 250);
    root->addWidget(shell);

    auto* layout = new QVBoxLayout(shell);
    layout->setContentsMargins(24, 18, 24, 18);
    layout->setSpacing(12);

    auto* title = new QLabel(titleText.toUpper(), shell);
    title->setAlignment(Qt::AlignCenter);
    title->setObjectName(QStringLiteral("noticeCardTitle"));
    layout->addWidget(title);

    layout->addStretch(1);

    auto* body = new QLabel(bodyText.toUpper(), shell);
    body->setAlignment(Qt::AlignCenter);
    body->setWordWrap(true);
    body->setObjectName(QStringLiteral("noticeCardBody"));
    layout->addWidget(body);

    layout->addStretch(1);

    auto* footerRow = new QHBoxLayout();
    footerRow->setSpacing(8);
    auto* leftLine = new QFrame(shell);
    leftLine->setFrameShape(QFrame::HLine);
    auto* footer = new QLabel(QStringLiteral("NIMONSPOLI"), shell);
    footer->setObjectName(QStringLiteral("noticeCardFooter"));
    auto* rightLine = new QFrame(shell);
    rightLine->setFrameShape(QFrame::HLine);
    footerRow->addWidget(leftLine, 1);
    footerRow->addWidget(footer, 0);
    footerRow->addWidget(rightLine, 1);
    layout->addLayout(footerRow);

    auto* continueButton = new QPushButton(buttonText.toUpper(), shell);
    continueButton->setCursor(Qt::PointingHandCursor);
    continueButton->setMinimumHeight(38);
    layout->addWidget(continueButton);

    dialog.setStyleSheet(QStringLiteral(
        "#noticeCardShell {"
        "  background: #fffef8;"
        "  border: 2px solid #1b1b1b;"
        "  border-radius: 2px;"
        "}"
        "#noticeCardTitle {"
        "  color: #090909;"
        "  font: 900 15pt 'Trebuchet MS';"
        "  letter-spacing: 0.5px;"
        "}"
        "#noticeCardBody {"
        "  color: #111;"
        "  font: 800 12pt 'Trebuchet MS';"
        "  line-height: 120%;"
        "}"
        "#noticeCardFooter {"
        "  color: #111;"
        "  font: 700 8pt 'Trebuchet MS';"
        "}"
        "QPushButton {"
        "  background: #111;"
        "  color: white;"
        "  border: none;"
        "  border-radius: 4px;"
        "  font: 900 9pt 'Trebuchet MS';"
        "  letter-spacing: 1px;"
        "}"
        "QPushButton:hover { background: #2a2a2a; }"
    ));

    QObject::connect(continueButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    centerDialogOnParent(dialog, parent);
    animateDialogEntrance(dialog);
    dialog.exec();
}

bool showChoiceCard(
    QWidget* parent,
    const QString& titleText,
    const QString& bodyText,
    const QString& acceptText,
    const QString& rejectText
)
{
    QDialog dialog(parent, Qt::Dialog | Qt::FramelessWindowHint);
    dialog.setModal(true);
    dialog.setAttribute(Qt::WA_TranslucentBackground);

    auto* root = new QVBoxLayout(&dialog);
    root->setContentsMargins(20, 20, 20, 20);

    auto* shell = new QFrame(&dialog);
    shell->setObjectName(QStringLiteral("choiceCardShell"));
    shell->setMinimumSize(430, 250);
    root->addWidget(shell);

    auto* layout = new QVBoxLayout(shell);
    layout->setContentsMargins(24, 18, 24, 18);
    layout->setSpacing(14);

    auto* title = new QLabel(titleText.toUpper(), shell);
    title->setAlignment(Qt::AlignCenter);
    title->setObjectName(QStringLiteral("choiceCardTitle"));
    layout->addWidget(title);

    auto* body = new QLabel(bodyText, shell);
    body->setAlignment(Qt::AlignCenter);
    body->setWordWrap(true);
    body->setObjectName(QStringLiteral("choiceCardBody"));
    layout->addWidget(body, 1);

    auto* buttonRow = new QHBoxLayout();
    buttonRow->setSpacing(10);
    layout->addLayout(buttonRow);

    auto* rejectButton = new QPushButton(rejectText.toUpper(), shell);
    rejectButton->setObjectName(QStringLiteral("choiceRejectButton"));
    rejectButton->setCursor(Qt::PointingHandCursor);
    rejectButton->setMinimumHeight(40);
    buttonRow->addWidget(rejectButton);

    auto* acceptButton = new QPushButton(acceptText.toUpper(), shell);
    acceptButton->setObjectName(QStringLiteral("choiceAcceptButton"));
    acceptButton->setCursor(Qt::PointingHandCursor);
    acceptButton->setMinimumHeight(40);
    buttonRow->addWidget(acceptButton);

    dialog.setStyleSheet(QStringLiteral(
        "#choiceCardShell { background:#fffef8; border:2px solid #1b1b1b; border-radius:2px; }"
        "#choiceCardTitle { color:#090909; font:900 15pt 'Trebuchet MS'; letter-spacing:0.5px; }"
        "#choiceCardBody { color:#111; font:800 11pt 'Trebuchet MS'; line-height:120%; }"
        "QPushButton { border-radius:4px; font:900 9pt 'Trebuchet MS'; letter-spacing:1px; }"
        "#choiceRejectButton { background:#e7e2d4; color:#111; border:1px solid #111; }"
        "#choiceRejectButton:hover { background:#f4efe2; }"
        "#choiceAcceptButton { background:#111; color:white; border:none; }"
        "#choiceAcceptButton:hover { background:#2a2a2a; }"
    ));

    bool accepted = false;
    QObject::connect(rejectButton, &QPushButton::clicked, &dialog, [&]() {
        accepted = false;
        dialog.reject();
    });
    QObject::connect(acceptButton, &QPushButton::clicked, &dialog, [&]() {
        accepted = true;
        dialog.accept();
    });

    centerDialogOnParent(dialog, parent);
    animateDialogEntrance(dialog);
    dialog.exec();
    return accepted;
}

int showNumberPrompt(
    QWidget* parent,
    const QString& titleText,
    const QString& promptText,
    int minValue,
    int maxValue,
    int initialValue,
    bool* accepted
)
{
    QDialog dialog(parent, Qt::Dialog | Qt::FramelessWindowHint);
    dialog.setModal(true);
    dialog.setAttribute(Qt::WA_TranslucentBackground);

    auto* root = new QVBoxLayout(&dialog);
    root->setContentsMargins(20, 20, 20, 20);

    auto* shell = new QFrame(&dialog);
    shell->setObjectName(QStringLiteral("numberPromptShell"));
    root->addWidget(shell);

    auto* layout = new QVBoxLayout(shell);
    layout->setContentsMargins(24, 20, 24, 20);
    layout->setSpacing(14);

    auto* title = new QLabel(titleText.toUpper(), shell);
    title->setAlignment(Qt::AlignCenter);
    title->setObjectName(QStringLiteral("numberPromptTitle"));
    layout->addWidget(title);

    auto* prompt = new QLabel(promptText, shell);
    prompt->setAlignment(Qt::AlignCenter);
    prompt->setWordWrap(true);
    prompt->setObjectName(QStringLiteral("numberPromptBody"));
    layout->addWidget(prompt);

    auto* input = new QSpinBox(shell);
    input->setObjectName(QStringLiteral("numberPromptInput"));
    input->setRange(minValue, maxValue);
    input->setValue(qBound(minValue, initialValue, maxValue));
    input->setSingleStep(1);
    layout->addWidget(input);

    auto* buttonRow = new QHBoxLayout();
    buttonRow->setSpacing(10);
    layout->addLayout(buttonRow);

    auto* cancelButton = new QPushButton(QStringLiteral("CANCEL"), shell);
    cancelButton->setObjectName(QStringLiteral("numberCancelButton"));
    cancelButton->setCursor(Qt::PointingHandCursor);
    cancelButton->setMinimumHeight(40);
    buttonRow->addWidget(cancelButton);

    auto* okButton = new QPushButton(QStringLiteral("OK"), shell);
    okButton->setObjectName(QStringLiteral("numberOkButton"));
    okButton->setCursor(Qt::PointingHandCursor);
    okButton->setMinimumHeight(40);
    buttonRow->addWidget(okButton);

    dialog.setStyleSheet(QStringLiteral(
        "#numberPromptShell { background:#fffef8; border:2px solid #111; border-radius:2px; }"
        "#numberPromptTitle { color:#090909; font:900 15pt 'Trebuchet MS'; letter-spacing:0.5px; }"
        "#numberPromptBody { color:#26333f; font:800 10pt 'Trebuchet MS'; }"
        "#numberPromptInput { min-height:42px; padding:4px 10px; border:2px solid #111; border-radius:4px; background:white; color:#111; font:900 13pt 'Trebuchet MS'; }"
        "QPushButton { border-radius:4px; font:900 9pt 'Trebuchet MS'; letter-spacing:1px; }"
        "#numberCancelButton { background:#e7e2d4; color:#111; border:1px solid #111; }"
        "#numberCancelButton:hover { background:#f4efe2; }"
        "#numberOkButton { background:#111; color:white; border:none; }"
        "#numberOkButton:hover { background:#2a2a2a; }"
    ));

    int value = input->value();
    *accepted = false;
    QObject::connect(cancelButton, &QPushButton::clicked, &dialog, [&]() {
        *accepted = false;
        dialog.reject();
    });
    QObject::connect(okButton, &QPushButton::clicked, &dialog, [&]() {
        value = input->value();
        *accepted = true;
        dialog.accept();
    });

    dialog.resize(450, 270);
    centerDialogOnParent(dialog, parent);
    animateDialogEntrance(dialog);
    dialog.exec();
    return value;
}

}  // namespace

QtGameIO::QtGameIO(QWidget* dialogParent)
    : dialogParent(dialogParent)
{
}

void QtGameIO::setDialogParent(QWidget* dialogParent)
{
    this->dialogParent = dialogParent;
}

void QtGameIO::setMovementStepHandler(std::function<void(const Player&, int)> handler)
{
    movementStepHandler = std::move(handler);
}

void QtGameIO::setPropertyPurchaseHandler(std::function<bool(const Player&, const PropertyTile&)> handler)
{
    propertyPurchaseHandler = std::move(handler);
}

void QtGameIO::setPropertyNoticeHandler(std::function<void(const Player&, const PropertyTile&)> handler)
{
    propertyNoticeHandler = std::move(handler);
}

void QtGameIO::setBoardTileSelectionHandler(std::function<int(const QString&, const QVector<int>&, bool)> handler)
{
    boardTileSelectionHandler = std::move(handler);
}

void QtGameIO::setLiquidationPlanHandler(std::function<bool(
    const Player&,
    int,
    const std::vector<LiquidationCandidate>&,
    std::vector<LiquidationDecision>&
)> handler)
{
    liquidationPlanHandler = std::move(handler);
}

int QtGameIO::promptInt(const std::string& prompt)
{
    if (isTestMode()) {
        Q_UNUSED(prompt);
        return 0;
    }

    bool accepted = false;
    const int value = showNumberPrompt(
        dialogParent,
        QStringLiteral("Input Angka"),
        toQString(prompt),
        0,
        std::numeric_limits<int>::max(),
        0,
        &accepted);

    if (!accepted) {
        throw std::runtime_error("Aksi dibatalkan oleh pengguna.");
    }

    return value;
}

int QtGameIO::promptIntInRange(const std::string& prompt, int minValue, int maxValue)
{
    if (isTestMode()) {
        Q_UNUSED(prompt);
        Q_UNUSED(maxValue);
        return minValue;
    }

    bool accepted = false;
    const int value = showNumberPrompt(
        dialogParent,
        QStringLiteral("Pilih Opsi"),
        toQString(prompt),
        minValue,
        maxValue,
        minValue,
        &accepted);

    if (!accepted) {
        throw std::runtime_error("Aksi dibatalkan oleh pengguna.");
    }

    return value;
}

bool QtGameIO::confirmYN(const std::string& message)
{
    if (isTestMode()) {
        Q_UNUSED(message);
        return false;
    }

    return showChoiceCard(
        dialogParent,
        QStringLiteral("Konfirmasi"),
        toQString(message),
        QStringLiteral("Yes"),
        QStringLiteral("No"));
}

void QtGameIO::showDiceRoll(int die1, int die2)
{
    if (isTestMode()) {
        return;
    }

    QDialog dialog(dialogParent, Qt::Dialog | Qt::FramelessWindowHint);
    dialog.setModal(true);
    dialog.setAttribute(Qt::WA_TranslucentBackground);

    auto* root = new QVBoxLayout(&dialog);
    root->setContentsMargins(18, 18, 18, 18);

    auto* shell = new QWidget(&dialog);
    shell->setObjectName(QStringLiteral("diceRollShell"));
    auto* shellLayout = new QVBoxLayout(shell);
    shellLayout->setContentsMargins(26, 22, 26, 22);
    shellLayout->setSpacing(12);
    root->addWidget(shell);

    auto* title = new QLabel(QStringLiteral("ROLLING DICE"), shell);
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet(QStringLiteral("color:#162432;font:900 12pt 'Trebuchet MS';letter-spacing:1px;"));
    shellLayout->addWidget(title);

    auto* diceLabel = new QLabel(shell);
    diceLabel->setAlignment(Qt::AlignCenter);
    shellLayout->addWidget(diceLabel);

    auto* totalLabel = new QLabel(shell);
    totalLabel->setAlignment(Qt::AlignCenter);
    totalLabel->setStyleSheet(QStringLiteral("color:#2f8c2f;font:900 18pt 'Trebuchet MS';"));
    shellLayout->addWidget(totalLabel);

    dialog.setStyleSheet(QStringLiteral(
        "#diceRollShell {"
        "  background: rgba(255,255,255,0.96);"
        "  border: 1px solid rgba(142,155,172,0.78);"
        "  border-radius: 22px;"
        "}"
    ));

    int ticks = 0;
    QTimer timer(&dialog);
    QObject::connect(&timer, &QTimer::timeout, &dialog, [&]() {
        ++ticks;
        const bool finalTick = ticks >= 13;
        const int left = finalTick ? die1 : QRandomGenerator::global()->bounded(1, 7);
        const int right = finalTick ? die2 : QRandomGenerator::global()->bounded(1, 7);
        diceLabel->setPixmap(makeDicePair(left, right));
        totalLabel->setText(finalTick
            ? QStringLiteral("%1 + %2 = %3").arg(die1).arg(die2).arg(die1 + die2)
            : QStringLiteral("..."));

        if (finalTick) {
            timer.stop();
            QTimer::singleShot(420, &dialog, &QDialog::accept);
        }
    });

    diceLabel->setPixmap(makeDicePair(
        QRandomGenerator::global()->bounded(1, 7),
        QRandomGenerator::global()->bounded(1, 7)
    ));
    totalLabel->setText(QStringLiteral("..."));

    dialog.adjustSize();
    if (dialogParent != nullptr) {
        const QPoint center = dialogParent->mapToGlobal(dialogParent->rect().center());
        dialog.move(center.x() - dialog.width() / 2, center.y() - dialog.height() / 2);
    }

    timer.start(58);
    dialog.exec();
}

void QtGameIO::showPawnStep(const Player& player, int tileIndex)
{
    if (movementStepHandler) {
        movementStepHandler(player, tileIndex);
    }

    if (isTestMode()) {
        return;
    }

    QEventLoop loop;
    QTimer::singleShot(140, &loop, &QEventLoop::quit);
    loop.exec();
}

bool QtGameIO::confirmPropertyPurchase(const Player& player, const PropertyTile& property)
{
    if (isTestMode()) {
        return false;
    }

    if (propertyPurchaseHandler) {
        return propertyPurchaseHandler(player, property);
    }

    return GameIO::confirmPropertyPurchase(player, property);
}

void QtGameIO::showPropertyNotice(const Player& player, const PropertyTile& property)
{
    if (isTestMode()) {
        return;
    }

    if (propertyNoticeHandler) {
        propertyNoticeHandler(player, property);
        return;
    }

    GameIO::showPropertyNotice(player, property);
}

void QtGameIO::showActionCard(CardType cardType, const ActionCard& card)
{
    pendingMessages.append(QStringLiteral("Kartu: \"%1\"").arg(toQString(card.getText())));

    if (isTestMode()) {
        return;
    }

    QDialog dialog(dialogParent, Qt::Dialog | Qt::FramelessWindowHint);
    dialog.setModal(true);
    dialog.setAttribute(Qt::WA_TranslucentBackground);

    auto* root = new QVBoxLayout(&dialog);
    root->setContentsMargins(20, 20, 20, 20);

    auto* shell = new QFrame(&dialog);
    shell->setObjectName(QStringLiteral("actionCardShell"));
    shell->setMinimumSize(430, 280);
    root->addWidget(shell);

    auto* layout = new QVBoxLayout(shell);
    layout->setContentsMargins(24, 18, 24, 18);
    layout->setSpacing(12);

    auto* title = new QLabel(actionCardTitle(cardType), shell);
    title->setAlignment(Qt::AlignCenter);
    title->setObjectName(QStringLiteral("actionCardTitle"));
    layout->addWidget(title);

    layout->addStretch(1);

    auto* body = new QLabel(toQString(card.getText()).toUpper(), shell);
    body->setAlignment(Qt::AlignCenter);
    body->setWordWrap(true);
    body->setObjectName(QStringLiteral("actionCardBody"));
    layout->addWidget(body);

    layout->addStretch(1);

    auto* footerRow = new QHBoxLayout();
    footerRow->setSpacing(8);
    auto* leftLine = new QFrame(shell);
    leftLine->setFrameShape(QFrame::HLine);
    auto* footer = new QLabel(QStringLiteral("NIMONSPOLI"), shell);
    footer->setObjectName(QStringLiteral("actionCardFooter"));
    auto* rightLine = new QFrame(shell);
    rightLine->setFrameShape(QFrame::HLine);
    footerRow->addWidget(leftLine, 1);
    footerRow->addWidget(footer, 0);
    footerRow->addWidget(rightLine, 1);
    layout->addLayout(footerRow);

    auto* continueButton = new QPushButton(QStringLiteral("CONTINUE"), shell);
    continueButton->setCursor(Qt::PointingHandCursor);
    continueButton->setMinimumHeight(38);
    layout->addWidget(continueButton);

    dialog.setStyleSheet(QStringLiteral(
        "#actionCardShell {"
        "  background: #fffef8;"
        "  border: 2px solid #1b1b1b;"
        "  border-radius: 2px;"
        "}"
        "#actionCardTitle {"
        "  color: #090909;"
        "  font: 900 15pt 'Trebuchet MS';"
        "  letter-spacing: 0.5px;"
        "}"
        "#actionCardBody {"
        "  color: #111;"
        "  font: 800 12pt 'Trebuchet MS';"
        "  line-height: 120%;"
        "}"
        "#actionCardFooter {"
        "  color: #111;"
        "  font: 700 8pt 'Trebuchet MS';"
        "}"
        "QPushButton {"
        "  background: #111;"
        "  color: white;"
        "  border: none;"
        "  border-radius: 4px;"
        "  font: 900 9pt 'Trebuchet MS';"
        "  letter-spacing: 1px;"
        "}"
        "QPushButton:hover { background: #2a2a2a; }"
    ));

    QObject::connect(continueButton, &QPushButton::clicked, &dialog, &QDialog::accept);

    dialog.adjustSize();
    if (dialogParent != nullptr) {
        const QPoint center = dialogParent->mapToGlobal(dialogParent->rect().center());
        dialog.move(center.x() - dialog.width() / 2, center.y() - dialog.height() / 2);
    }
    animateDialogEntrance(dialog);
    dialog.exec();
}

void QtGameIO::showPaymentNotification(const std::string& title, const std::string& detail)
{
    if (isTestMode()) {
        return;
    }

    showNoticeCard(dialogParent, toQString(title), toQString(detail), QStringLiteral("CONTINUE"));
}

void QtGameIO::showAuctionNotification(const std::string& title, const std::string& detail)
{
    if (isTestMode()) {
        return;
    }

    showNoticeCard(dialogParent, toQString(title), toQString(detail), QStringLiteral("CONTINUE"));
}

int QtGameIO::promptAuctionBid(const PropertyTile& property, const Player& bidder, int highestBid)
{
    if (isTestMode()) {
        Q_UNUSED(property);
        Q_UNUSED(bidder);
        Q_UNUSED(highestBid);
        return 0;
    }

    QDialog dialog(dialogParent, Qt::Dialog | Qt::FramelessWindowHint);
    dialog.setModal(true);
    dialog.setAttribute(Qt::WA_TranslucentBackground);

    auto* root = new QVBoxLayout(&dialog);
    root->setContentsMargins(20, 20, 20, 20);

    auto* shell = new QFrame(&dialog);
    shell->setObjectName(QStringLiteral("auctionShell"));
    root->addWidget(shell);

    auto* layout = new QVBoxLayout(shell);
    layout->setContentsMargins(24, 20, 24, 20);
    layout->setSpacing(14);

    auto* title = new QLabel(QStringLiteral("AUCTION"), shell);
    title->setAlignment(Qt::AlignCenter);
    title->setObjectName(QStringLiteral("auctionTitle"));
    layout->addWidget(title);

    auto* propertyLabel = new QLabel(
        QStringLiteral("%1 (%2)")
            .arg(toQString(property.getName()))
            .arg(toQString(property.getCode())),
        shell
    );
    propertyLabel->setAlignment(Qt::AlignCenter);
    propertyLabel->setWordWrap(true);
    propertyLabel->setObjectName(QStringLiteral("auctionProperty"));
    layout->addWidget(propertyLabel);

    auto* bidderLabel = new QLabel(
        QStringLiteral("%1\nSaldo: M%2 | Bid tertinggi: M%3")
            .arg(toQString(bidder.getUsername()))
            .arg(bidder.getBalance())
            .arg(qMax(0, highestBid)),
        shell
    );
    bidderLabel->setAlignment(Qt::AlignCenter);
    bidderLabel->setWordWrap(true);
    bidderLabel->setObjectName(QStringLiteral("auctionBidder"));
    layout->addWidget(bidderLabel);

    auto* bidInput = new QSpinBox(shell);
    bidInput->setObjectName(QStringLiteral("auctionBidInput"));
    bidInput->setRange(0, qMax(0, bidder.getBalance()));
    bidInput->setSingleStep(10);
    bidInput->setPrefix(QStringLiteral("M"));
    bidInput->setValue(highestBid < 0 ? 0 : qMin(bidder.getBalance(), highestBid + 1));
    bidInput->setEnabled(highestBid < 0 || bidder.getBalance() > highestBid);
    layout->addWidget(bidInput);

    auto* hint = new QLabel(
        highestBid < 0 || bidder.getBalance() > highestBid
            ? QStringLiteral("Masukkan bid lebih tinggi dari bid saat ini, atau pass.")
            : QStringLiteral("Saldo tidak cukup untuk menaikkan bid. Pilih pass."),
        shell
    );
    hint->setAlignment(Qt::AlignCenter);
    hint->setWordWrap(true);
    hint->setObjectName(QStringLiteral("auctionHint"));
    layout->addWidget(hint);

    auto* buttonRow = new QHBoxLayout();
    buttonRow->setSpacing(10);
    layout->addLayout(buttonRow);

    auto* passButton = new QPushButton(QStringLiteral("PASS"), shell);
    passButton->setObjectName(QStringLiteral("auctionPassButton"));
    passButton->setCursor(Qt::PointingHandCursor);
    passButton->setMinimumHeight(42);
    buttonRow->addWidget(passButton);

    auto* bidButton = new QPushButton(QStringLiteral("BID"), shell);
    bidButton->setObjectName(QStringLiteral("auctionBidButton"));
    bidButton->setCursor(Qt::PointingHandCursor);
    bidButton->setMinimumHeight(42);
    bidButton->setEnabled(highestBid < 0 || bidder.getBalance() > highestBid);
    buttonRow->addWidget(bidButton);

    dialog.setStyleSheet(QStringLiteral(
        "#auctionShell {"
        "  background: #fffef8;"
        "  border: 2px solid #111;"
        "  border-radius: 2px;"
        "}"
        "#auctionTitle {"
        "  color:#090909;"
        "  font:900 16pt 'Trebuchet MS';"
        "  letter-spacing:0.5px;"
        "}"
        "#auctionProperty { color:#111; font:900 12pt 'Trebuchet MS'; }"
        "#auctionBidder { color:#26333f; font:800 10pt 'Trebuchet MS'; }"
        "#auctionHint { color:#5d6670; font:700 8.8pt 'Trebuchet MS'; }"
        "#auctionBidInput {"
        "  min-height: 42px;"
        "  padding: 4px 10px;"
        "  border: 2px solid #111;"
        "  border-radius: 4px;"
        "  background: white;"
        "  color:#111;"
        "  font:900 13pt 'Trebuchet MS';"
        "}"
        "QPushButton {"
        "  border: none;"
        "  border-radius: 4px;"
        "  font: 900 9pt 'Trebuchet MS';"
        "  letter-spacing: 1px;"
        "}"
        "#auctionPassButton { background:#e7e2d4; color:#111; border:1px solid #111; }"
        "#auctionPassButton:hover { background:#f4efe2; }"
        "#auctionBidButton { background:#111; color:white; }"
        "#auctionBidButton:hover { background:#2a2a2a; }"
        "#auctionBidButton:disabled { background:#c9c5b9; color:#777; }"
    ));

    int selectedBid = 0;
    QObject::connect(passButton, &QPushButton::clicked, &dialog, [&]() {
        selectedBid = -1;
        dialog.reject();
    });
    QObject::connect(bidButton, &QPushButton::clicked, &dialog, [&]() {
        selectedBid = bidInput->value();
        dialog.accept();
    });

    dialog.resize(450, 330);
    centerDialogOnParent(dialog, dialogParent);
    animateDialogEntrance(dialog);
    dialog.exec();
    return selectedBid;
}

int QtGameIO::promptTaxPaymentOption(
    const Player& player,
    const std::string& tileName,
    int flatAmount,
    int percentage,
    int wealth,
    int percentageAmount
)
{
    if (isTestMode()) {
        Q_UNUSED(player);
        Q_UNUSED(tileName);
        Q_UNUSED(flatAmount);
        Q_UNUSED(percentage);
        Q_UNUSED(wealth);
        Q_UNUSED(percentageAmount);
        return 1;
    }

    QDialog dialog(dialogParent, Qt::Dialog | Qt::FramelessWindowHint);
    dialog.setModal(true);
    dialog.setAttribute(Qt::WA_TranslucentBackground);

    auto* root = new QVBoxLayout(&dialog);
    root->setContentsMargins(20, 20, 20, 20);

    auto* shell = new QFrame(&dialog);
    shell->setObjectName(QStringLiteral("taxShell"));
    root->addWidget(shell);

    auto* layout = new QVBoxLayout(shell);
    layout->setContentsMargins(24, 20, 24, 20);
    layout->setSpacing(14);

    auto* title = new QLabel(QStringLiteral("PPH"), shell);
    title->setAlignment(Qt::AlignCenter);
    title->setObjectName(QStringLiteral("taxTitle"));
    layout->addWidget(title);

    auto* body = new QLabel(
        QStringLiteral("%1 mendarat di %2.\nPilih metode pembayaran pajak.")
            .arg(toQString(player.getUsername()))
            .arg(toQString(tileName)),
        shell
    );
    body->setAlignment(Qt::AlignCenter);
    body->setWordWrap(true);
    body->setObjectName(QStringLiteral("taxBody"));
    layout->addWidget(body);

    auto* wealthLabel = new QLabel(
        QStringLiteral("Total kekayaan: M%1").arg(wealth),
        shell
    );
    wealthLabel->setAlignment(Qt::AlignCenter);
    wealthLabel->setObjectName(QStringLiteral("taxWealth"));
    layout->addWidget(wealthLabel);

    auto* flatButton = new QPushButton(
        QStringLiteral("BAYAR TETAP - M%1").arg(flatAmount),
        shell
    );
    flatButton->setObjectName(QStringLiteral("taxPrimaryButton"));
    flatButton->setCursor(Qt::PointingHandCursor);
    flatButton->setMinimumHeight(48);
    layout->addWidget(flatButton);

    auto* percentButton = new QPushButton(
        QStringLiteral("BAYAR %1% DARI KEKAYAAN - M%2")
            .arg(percentage)
            .arg(percentageAmount),
        shell
    );
    percentButton->setObjectName(QStringLiteral("taxSecondaryButton"));
    percentButton->setCursor(Qt::PointingHandCursor);
    percentButton->setMinimumHeight(48);
    layout->addWidget(percentButton);

    auto* hint = new QLabel(QStringLiteral("Pilihan ini menentukan pajak yang langsung dibayarkan ke Bank."), shell);
    hint->setAlignment(Qt::AlignCenter);
    hint->setWordWrap(true);
    hint->setObjectName(QStringLiteral("taxHint"));
    layout->addWidget(hint);

    dialog.setStyleSheet(QStringLiteral(
        "#taxShell {"
        "  background: #fffef8;"
        "  border: 2px solid #111;"
        "  border-radius: 2px;"
        "}"
        "#taxTitle { color:#090909; font:900 16pt 'Trebuchet MS'; letter-spacing:0.5px; }"
        "#taxBody { color:#17232d; font:800 10.5pt 'Trebuchet MS'; line-height:120%; }"
        "#taxWealth { color:#2f8c2f; font:900 12pt 'Trebuchet MS'; }"
        "#taxHint { color:#5d6670; font:700 8.8pt 'Trebuchet MS'; }"
        "QPushButton { border-radius:4px; font:900 9.5pt 'Trebuchet MS'; letter-spacing:1px; }"
        "#taxPrimaryButton { background:#111; color:white; border:none; }"
        "#taxPrimaryButton:hover { background:#2a2a2a; }"
        "#taxSecondaryButton { background:#e7e2d4; color:#111; border:1px solid #111; }"
        "#taxSecondaryButton:hover { background:#f4efe2; }"
    ));

    int selectedChoice = 1;
    QObject::connect(flatButton, &QPushButton::clicked, &dialog, [&]() {
        selectedChoice = 1;
        dialog.accept();
    });
    QObject::connect(percentButton, &QPushButton::clicked, &dialog, [&]() {
        selectedChoice = 2;
        dialog.accept();
    });

    dialog.resize(460, 340);
    centerDialogOnParent(dialog, dialogParent);
    animateDialogEntrance(dialog);
    dialog.exec();
    return selectedChoice;
}

bool QtGameIO::promptLiquidationPlan(
    const Player& player,
    int targetAmount,
    const std::vector<LiquidationCandidate>& candidates,
    std::vector<LiquidationDecision>& decisions
)
{
    if (isTestMode()) {
        Q_UNUSED(player);
        Q_UNUSED(targetAmount);
        decisions.clear();
        for (const LiquidationCandidate& candidate : candidates) {
            if (candidate.sellValue > 0) {
                decisions.push_back({candidate.tileIndex, LiquidationActionKind::Sell});
                return true;
            }
            if (candidate.mortgageValue > 0) {
                decisions.push_back({candidate.tileIndex, LiquidationActionKind::Mortgage});
                return true;
            }
        }
        return false;
    }

    if (liquidationPlanHandler) {
        return liquidationPlanHandler(player, targetAmount, candidates, decisions);
    }

    return GameIO::promptLiquidationPlan(player, targetAmount, candidates, decisions);
}

int QtGameIO::promptTileSelection(const std::string& title, const std::vector<int>& validTileIndices)
{
    return promptTileSelection(title, validTileIndices, false);
}

int QtGameIO::promptTileSelection(
    const std::string& title,
    const std::vector<int>& validTileIndices,
    bool allowCancel
)
{
    if (isTestMode()) {
        return allowCancel ? -1 : (validTileIndices.empty() ? -1 : validTileIndices.front());
    }

    if (boardTileSelectionHandler) {
        QVector<int> indices;
        indices.reserve(static_cast<int>(validTileIndices.size()));
        for (int index : validTileIndices) {
            indices.append(index);
        }
        return boardTileSelectionHandler(toQString(title), indices, allowCancel);
    }

    return GameIO::promptTileSelection(title, validTileIndices, allowCancel);
}

int QtGameIO::promptSkillCardSelection(
    const std::string& title,
    const std::vector<SkillCard*>& cards,
    bool allowCancel
)
{
    if (isTestMode()) {
        Q_UNUSED(title);
        return allowCancel ? 0 : 1;
    }

    QDialog dialog(dialogParent, Qt::Dialog | Qt::FramelessWindowHint);
    dialog.setModal(true);
    dialog.setAttribute(Qt::WA_TranslucentBackground);

    auto* root = new QVBoxLayout(&dialog);
    root->setContentsMargins(18, 18, 18, 18);

    auto* shell = new QFrame(&dialog);
    shell->setObjectName(QStringLiteral("skillPickerShell"));
    root->addWidget(shell);

    auto* layout = new QVBoxLayout(shell);
    layout->setContentsMargins(24, 22, 24, 22);
    layout->setSpacing(16);

    auto* header = new QLabel(toQString(title).toUpper(), shell);
    header->setAlignment(Qt::AlignCenter);
    header->setObjectName(QStringLiteral("skillPickerTitle"));
    layout->addWidget(header);

    auto* subtitle = new QLabel(
        allowCancel
            ? QStringLiteral("Pilih satu kartu untuk digunakan, atau batal.")
            : QStringLiteral("Hand penuh. Pilih satu kartu yang akan dibuang."),
        shell
    );
    subtitle->setAlignment(Qt::AlignCenter);
    subtitle->setWordWrap(true);
    subtitle->setObjectName(QStringLiteral("skillPickerSubtitle"));
    layout->addWidget(subtitle);

    auto* scroll = new QScrollArea(shell);
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setFrameShape(QFrame::NoFrame);
    layout->addWidget(scroll);

    auto* cardsHost = new QWidget(scroll);
    auto* cardsLayout = new QHBoxLayout(cardsHost);
    cardsLayout->setContentsMargins(6, 10, 6, 10);
    cardsLayout->setSpacing(14);
    scroll->setWidget(cardsHost);

    int selected = allowCancel ? 0 : 1;
    for (int i = 0; i < static_cast<int>(cards.size()); ++i) {
        SkillCard* card = cards[static_cast<std::size_t>(i)];
        const QString description = card == nullptr
            ? QStringLiteral("Kartu kemampuan")
            : QString::fromStdString(DeckFactory::describeSkillCard(card));

        auto* cardButton = new QPushButton(cardsHost);
        cardButton->setObjectName(QStringLiteral("skillCardButton"));
        cardButton->setCursor(Qt::PointingHandCursor);
        cardButton->setMinimumSize(168, 224);
        cardButton->setMaximumSize(188, 240);
        cardButton->setText(QStringLiteral("%1\n\n%2\n\n#%3")
            .arg(skillCardTitle(card), description, QString::number(i + 1)));
        cardsLayout->addWidget(cardButton);

        QObject::connect(cardButton, &QPushButton::clicked, &dialog, [&, i]() {
            selected = i + 1;
            QTimer::singleShot(120, &dialog, &QDialog::accept);
        });
    }
    cardsLayout->addStretch(1);

    if (allowCancel) {
        auto* cancelButton = new QPushButton(QStringLiteral("BATAL"), shell);
        cancelButton->setObjectName(QStringLiteral("cancelSkillPickerButton"));
        cancelButton->setCursor(Qt::PointingHandCursor);
        cancelButton->setMinimumHeight(42);
        layout->addWidget(cancelButton);
        QObject::connect(cancelButton, &QPushButton::clicked, &dialog, [&]() {
            selected = 0;
            dialog.reject();
        });
    }

    dialog.setStyleSheet(QStringLiteral(
        "#skillPickerShell {"
        "  background: rgba(245, 240, 226, 0.98);"
        "  border: 1px solid rgba(32, 32, 32, 0.34);"
        "  border-radius: 20px;"
        "}"
        "#skillPickerTitle {"
        "  color: #101010;"
        "  font: 900 16pt 'Trebuchet MS';"
        "  letter-spacing: 1px;"
        "}"
        "#skillPickerSubtitle {"
        "  color: #31404b;"
        "  font: 700 9.5pt 'Trebuchet MS';"
        "}"
        "QScrollArea { background: transparent; }"
        "#skillCardButton {"
        "  text-align: center;"
        "  background: #fffef9;"
        "  color: #101010;"
        "  border: 2px solid #191919;"
        "  border-radius: 4px;"
        "  padding: 18px 12px;"
        "  font: 800 10pt 'Trebuchet MS';"
        "}"
        "#skillCardButton:hover {"
        "  background: #eaf6ff;"
        "  border: 3px solid #1262c5;"
        "}"
        "#cancelSkillPickerButton {"
        "  background: #20252b;"
        "  color: white;"
        "  border: none;"
        "  border-radius: 10px;"
        "  font: 900 10pt 'Trebuchet MS';"
        "  letter-spacing: 1px;"
        "}"
        "#cancelSkillPickerButton:hover { background: #343b45; }"
    ));

    dialog.resize(qMin(860, 220 + static_cast<int>(cards.size()) * 190), 370);
    if (dialogParent != nullptr) {
        const QPoint center = dialogParent->mapToGlobal(dialogParent->rect().center());
        dialog.move(center.x() - dialog.width() / 2, center.y() - dialog.height() / 2);
    }
    animateDialogEntrance(dialog);
    dialog.exec();
    return selected;
}

void QtGameIO::showMessage(const std::string& message)
{
    pendingMessages.append(toQString(message));
}

void QtGameIO::showError(
    const std::exception& exception,
    TransactionLogger* logger,
    int turn,
    const std::string& username
)
{
    const QString detail = toQString(exception.what());
    pendingMessages.append(detail);
    logErrorIfPossible(logger, turn, username, exception.what());

    if (isTestMode()) {
        return;
    }

    showNoticeCard(
        dialogParent,
        QStringLiteral("Terjadi Kesalahan"),
        detail,
        QStringLiteral("OK"));
}

QStringList QtGameIO::takePendingMessages()
{
    const QStringList messages = pendingMessages;
    pendingMessages.clear();
    return messages;
}

QString QtGameIO::latestMessage() const
{
    if (pendingMessages.isEmpty()) {
        return {};
    }

    return pendingMessages.back();
}

void QtGameIO::clearMessages()
{
    pendingMessages.clear();
}
