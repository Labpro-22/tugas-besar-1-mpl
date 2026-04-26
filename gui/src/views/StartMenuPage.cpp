#include "views/StartMenuPage.hpp"

#include <algorithm>

#include <QFont>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QResizeEvent>
#include <QStyle>
#include <QToolButton>
#include <QVBoxLayout>

namespace {

QPixmap makeBrandIcon(const QSize& size)
{
    QPixmap pixmap(size);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(QColor(10, 16, 24), qMax(2.0, size.width() * 0.055), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(Qt::NoBrush);

    const QRectF leftTower(size.width() * 0.12, size.height() * 0.18, size.width() * 0.30, size.height() * 0.60);
    const QRectF rightTower(size.width() * 0.48, size.height() * 0.34, size.width() * 0.26, size.height() * 0.44);
    painter.drawRect(leftTower);
    painter.drawRect(rightTower);

    painter.drawLine(
        QPointF(rightTower.left(), rightTower.center().y()),
        QPointF(rightTower.right(), rightTower.center().y())
    );
    painter.drawLine(
        QPointF(rightTower.center().x(), rightTower.top()),
        QPointF(rightTower.center().x(), rightTower.bottom())
    );

    const qreal windowSize = size.width() * 0.06;
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 2; ++col) {
            const QRectF windowRect(
                leftTower.left() + size.width() * 0.06 + col * size.width() * 0.11,
                leftTower.top() + size.height() * 0.08 + row * size.height() * 0.12,
                windowSize,
                windowSize
            );
            painter.drawRect(windowRect);
        }
    }

    return pixmap;
}

void paintSoftBuilding(QPainter& painter, const QPainterPath& path, const QColor& fill, qreal opacity)
{
    painter.save();
    painter.setOpacity(opacity);
    painter.fillPath(path, fill);
    painter.restore();
}

}  // namespace

StartMenuPage::StartMenuPage(QWidget* parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("startMenuPage"));

    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    auto* centerLayout = new QVBoxLayout();
    centerLayout->setContentsMargins(0, 44, 0, 44);
    centerLayout->setSpacing(20);
    centerLayout->setAlignment(Qt::AlignCenter);
    rootLayout->addLayout(centerLayout, 1);

    auto* iconLabel = new QLabel(this);
    iconLabel->setPixmap(makeBrandIcon(QSize(96, 96)));
    iconLabel->setAlignment(Qt::AlignCenter);
    centerLayout->addWidget(iconLabel, 0, Qt::AlignHCenter);

    titleLabel = new QLabel(QStringLiteral("Nimonspoli"), this);
    titleLabel->setAlignment(Qt::AlignCenter);
    QFont titleFont(QStringLiteral("Trebuchet MS"), 34, QFont::Black);
    titleLabel->setFont(titleFont);
    centerLayout->addWidget(titleLabel);

    subtitleLabel = new QLabel(QStringLiteral("Corporate Modern Edge"), this);
    subtitleLabel->setAlignment(Qt::AlignCenter);
    subtitleLabel->setStyleSheet(QStringLiteral("color:#3c4857;font:700 15pt 'Trebuchet MS';"));
    centerLayout->addWidget(subtitleLabel);

    auto* buttonStack = new QVBoxLayout();
    buttonStack->setSpacing(12);
    buttonStack->setContentsMargins(0, 26, 0, 10);
    centerLayout->addLayout(buttonStack);

    newGameButton = new QPushButton(style()->standardIcon(QStyle::SP_MediaPlay), QStringLiteral("NEW GAME"), this);
    newGameButton->setCursor(Qt::PointingHandCursor);
    newGameButton->setMinimumSize(400, 74);
    newGameButton->setIconSize(QSize(24, 24));
    buttonStack->addWidget(newGameButton, 0, Qt::AlignHCenter);

    loadGameButton = new QPushButton(style()->standardIcon(QStyle::SP_DirOpenIcon), QStringLiteral("LOAD GAME"), this);
    loadGameButton->setCursor(Qt::PointingHandCursor);
    loadGameButton->setMinimumSize(400, 74);
    loadGameButton->setIconSize(QSize(24, 24));
    buttonStack->addWidget(loadGameButton, 0, Qt::AlignHCenter);

    auto* utilityRow = new QHBoxLayout();
    utilityRow->setContentsMargins(0, 14, 0, 0);
    utilityRow->setSpacing(20);
    utilityRow->setAlignment(Qt::AlignCenter);
    centerLayout->addLayout(utilityRow);

    settingsButton = new QToolButton(this);
    settingsButton->setCursor(Qt::PointingHandCursor);
    settingsButton->setText(QStringLiteral("SETTINGS"));
    settingsButton->setToolButtonStyle(Qt::ToolButtonTextOnly);
    utilityRow->addWidget(settingsButton);

    leaderboardButton = new QToolButton(this);
    leaderboardButton->setCursor(Qt::PointingHandCursor);
    leaderboardButton->setText(QStringLiteral("LEADERBOARD"));
    leaderboardButton->setToolButtonStyle(Qt::ToolButtonTextOnly);
    utilityRow->addWidget(leaderboardButton);

    auto* shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(42);
    shadow->setOffset(0, 18);
    shadow->setColor(QColor(20, 32, 54, 34));
    newGameButton->setGraphicsEffect(shadow);

    setStyleSheet(
        "#startMenuPage { background: transparent; }"
        "QPushButton {"
        "  border-radius: 14px;"
        "  font: 900 11pt 'Trebuchet MS';"
        "  letter-spacing: 1px;"
        "  padding: 12px 20px;"
        "}"
        "QPushButton:hover { background-position: center; }"
        "QPushButton:pressed { padding-top: 14px; }"
        "QPushButton[text='NEW GAME'] {"
        "  background: #1159c7;"
        "  color: white;"
        "  border: 1px solid #0d4aa8;"
        "}"
        "QPushButton[text='LOAD GAME'] {"
        "  background: rgba(255,255,255,0.72);"
        "  color: #111820;"
        "  border: 1px solid rgba(90, 101, 120, 0.34);"
        "}"
        "QToolButton {"
        "  color: #2d3745;"
        "  background: transparent;"
        "  border: none;"
        "  font: 700 10pt 'Trebuchet MS';"
        "}"
        "QToolButton:hover { color: #1159c7; }"
    );

    connect(newGameButton, &QPushButton::clicked, this, &StartMenuPage::newGameRequested);
    connect(loadGameButton, &QPushButton::clicked, this, &StartMenuPage::loadGameRequested);
    connect(settingsButton, &QToolButton::clicked, this, &StartMenuPage::settingsRequested);
    connect(leaderboardButton, &QToolButton::clicked, this, &StartMenuPage::leaderboardRequested);

    updateResponsiveLayout();
}

void StartMenuPage::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QLinearGradient background(rect().topLeft(), rect().bottomLeft());
    background.setColorAt(0.0, QColor(244, 248, 252));
    background.setColorAt(0.55, QColor(248, 249, 251));
    background.setColorAt(1.0, QColor(239, 243, 248));
    painter.fillRect(rect(), background);

    painter.setBrush(QColor(255, 255, 255, 175));
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(QPointF(width() * 0.18, height() * 0.16), width() * 0.30, height() * 0.18);
    painter.drawEllipse(QPointF(width() * 0.78, height() * 0.22), width() * 0.22, height() * 0.14);

    QPainterPath leftTower;
    leftTower.moveTo(width() * 0.04, height());
    leftTower.lineTo(width() * 0.18, height() * 0.10);
    leftTower.lineTo(width() * 0.30, height() * 0.10);
    leftTower.lineTo(width() * 0.22, height());
    leftTower.closeSubpath();
    paintSoftBuilding(painter, leftTower, QColor(225, 231, 238), 0.58);

    QPainterPath centerTower;
    centerTower.moveTo(width() * 0.56, height());
    centerTower.lineTo(width() * 0.66, height() * 0.28);
    centerTower.lineTo(width() * 0.79, height() * 0.10);
    centerTower.lineTo(width() * 0.86, height() * 0.42);
    centerTower.lineTo(width() * 0.82, height());
    centerTower.closeSubpath();
    paintSoftBuilding(painter, centerTower, QColor(214, 222, 232), 0.60);

    QPainterPath rightTower;
    rightTower.moveTo(width() * 0.88, height());
    rightTower.lineTo(width() * 0.92, height() * 0.42);
    rightTower.lineTo(width() * 0.99, height() * 0.38);
    rightTower.lineTo(width(), height());
    rightTower.closeSubpath();
    paintSoftBuilding(painter, rightTower, QColor(231, 236, 242), 0.62);

    painter.save();
    painter.setClipPath(centerTower);
    painter.setPen(QPen(QColor(255, 255, 255, 110), 2));
    for (int index = 0; index < 18; ++index) {
        const qreal y = height() * 0.32 + index * height() * 0.032;
        painter.drawLine(
            QPointF(width() * 0.64, y),
            QPointF(width() * 0.84, y + height() * 0.06)
        );
    }
    painter.restore();
}

void StartMenuPage::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    updateResponsiveLayout();
}

void StartMenuPage::updateResponsiveLayout()
{
    const int buttonWidth = std::clamp(width() - 120, 280, 400);
    const int buttonHeight = std::clamp(height() / 11, 56, 74);
    const int iconSize = std::clamp(buttonHeight / 3, 18, 24);
    const int titleSize = std::clamp(width() / 28, 24, 34);
    const int subtitleSize = std::clamp(width() / 64, 11, 15);

    if (newGameButton != nullptr) {
        newGameButton->setMinimumSize(buttonWidth, buttonHeight);
        newGameButton->setMaximumWidth(buttonWidth);
        newGameButton->setIconSize(QSize(iconSize, iconSize));
    }

    if (loadGameButton != nullptr) {
        loadGameButton->setMinimumSize(buttonWidth, buttonHeight);
        loadGameButton->setMaximumWidth(buttonWidth);
        loadGameButton->setIconSize(QSize(iconSize, iconSize));
    }

    if (titleLabel != nullptr) {
        QFont titleFont(QStringLiteral("Trebuchet MS"), titleSize, QFont::Black);
        titleLabel->setFont(titleFont);
    }

    if (subtitleLabel != nullptr) {
        subtitleLabel->setStyleSheet(QStringLiteral(
            "color:#3c4857;font:700 %1pt 'Trebuchet MS';"
        ).arg(subtitleSize));
    }
}
