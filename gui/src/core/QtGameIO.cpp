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
#include <QInputDialog>
#include <QLabel>
#include <QMessageBox>
#include <QPainter>
#include <QPixmap>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QRandomGenerator>
#include <QScrollArea>
#include <QString>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

#include "core/DeckFactory.hpp"
#include "models/Enums.hpp"
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

int QtGameIO::promptInt(const std::string& prompt)
{
    if (isTestMode()) {
        Q_UNUSED(prompt);
        return 0;
    }

    bool accepted = false;
    const int value = QInputDialog::getInt(
        dialogParent,
        QStringLiteral("Input Angka"),
        toQString(prompt),
        0,
        0,
        std::numeric_limits<int>::max(),
        1,
        &accepted
    );

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
    const int value = QInputDialog::getInt(
        dialogParent,
        QStringLiteral("Pilih Opsi"),
        toQString(prompt),
        minValue,
        minValue,
        maxValue,
        1,
        &accepted
    );

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

    const QMessageBox::StandardButton result = QMessageBox::question(
        dialogParent,
        QStringLiteral("Konfirmasi"),
        toQString(message),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::Yes
    );

    return result == QMessageBox::Yes;
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

    QMessageBox::warning(
        dialogParent,
        QStringLiteral("Terjadi Kesalahan"),
        detail
    );
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
