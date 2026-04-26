#include "views/PropertyPortfolioWidget.hpp"

#include <algorithm>
#include <array>

#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>

#include "utils/UiCommon.hpp"

namespace {

struct GroupLayout {
    enum class Kind {
        Brown,
        LightBlue,
        Pink,
        Orange,
        Red,
        Yellow,
        Green,
        DarkBlue,
        Utilities,
        Railroads
    };

    Kind kind;
    qreal xRatio;
    qreal yRatio;
    int slotCount;
    bool compact = false;
};

constexpr std::array<GroupLayout, 10> kLayouts = {{
    {GroupLayout::Kind::Brown, 0.03, 0.03, 2, false},
    {GroupLayout::Kind::LightBlue, 0.03, 0.19, 3, false},
    {GroupLayout::Kind::Pink, 0.03, 0.37, 3, false},
    {GroupLayout::Kind::Orange, 0.03, 0.59, 3, false},
    {GroupLayout::Kind::Red, 0.03, 0.78, 3, false},
    {GroupLayout::Kind::Yellow, 0.61, 0.03, 3, false},
    {GroupLayout::Kind::Green, 0.61, 0.19, 3, false},
    {GroupLayout::Kind::DarkBlue, 0.69, 0.39, 2, false},
    {GroupLayout::Kind::Utilities, 0.69, 0.59, 2, false},
    {GroupLayout::Kind::Railroads, 0.54, 0.79, 4, true}
}};

QColor groupAccentFromColorGroup(ColorGroup group)
{
    return MonopolyUi::colorFromGroup(group, QColor(148, 194, 214));
}

QColor fadedAccent(const QColor& color)
{
    return QColor(color.red(), color.green(), color.blue(), 118);
}

QColor frameFill(const QColor& color)
{
    return QColor(color.red(), color.green(), color.blue(), 24);
}

GroupLayout::Kind groupKindFor(const PortfolioPropertyView& property)
{
    if (property.propertyType == PropertyType::RAILROAD) {
        return GroupLayout::Kind::Railroads;
    }
    if (property.propertyType == PropertyType::UTILITY) {
        return GroupLayout::Kind::Utilities;
    }

    switch (property.colorGroup) {
    case ColorGroup::COKLAT:
        return GroupLayout::Kind::Brown;
    case ColorGroup::BIRU_MUDA:
        return GroupLayout::Kind::LightBlue;
    case ColorGroup::MERAH_MUDA:
        return GroupLayout::Kind::Pink;
    case ColorGroup::ORANGE:
        return GroupLayout::Kind::Orange;
    case ColorGroup::MERAH:
        return GroupLayout::Kind::Red;
    case ColorGroup::KUNING:
        return GroupLayout::Kind::Yellow;
    case ColorGroup::HIJAU:
        return GroupLayout::Kind::Green;
    case ColorGroup::BIRU_TUA:
        return GroupLayout::Kind::DarkBlue;
    case ColorGroup::ABU_ABU:
    case ColorGroup::DEFAULT:
    default:
        return GroupLayout::Kind::Utilities;
    }
}

QString specialGlyph(const PortfolioPropertyView& property)
{
    if (property.propertyType == PropertyType::UTILITY) {
        return property.code == QStringLiteral("PLN") ? QStringLiteral("P") : QStringLiteral("W");
    }

    if (property.propertyType == PropertyType::RAILROAD) {
        return property.code.left(3).toUpper();
    }

    return QString();
}

}  // namespace

PropertyPortfolioWidget::PropertyPortfolioWidget(QWidget *parent)
    : QWidget(parent)
{
    setMinimumHeight(150);
    setMinimumWidth(240);
    setMouseTracking(true);
}

void PropertyPortfolioWidget::setPortfolio(const QVector<PortfolioPropertyView>& entries)
{
    portfolio = entries;
    std::sort(portfolio.begin(), portfolio.end(), [](const PortfolioPropertyView& left, const PortfolioPropertyView& right) {
        return left.propertyId < right.propertyId;
    });
    update();
}

void PropertyPortfolioWidget::setSelectedPropertyId(int propertyId)
{
    if (selectedPropertyId == propertyId) {
        return;
    }

    selectedPropertyId = propertyId;
    update();
}

QSize PropertyPortfolioWidget::minimumSizeHint() const
{
    return QSize(240, 150);
}

QSize PropertyPortfolioWidget::sizeHint() const
{
    return QSize(260, 176);
}

QColor PropertyPortfolioWidget::accentColorFor(const PortfolioPropertyView& property) const
{
    if (property.propertyType == PropertyType::RAILROAD) {
        return QColor(148, 194, 214);
    }
    if (property.propertyType == PropertyType::UTILITY) {
        return QColor(160, 203, 223);
    }

    return groupAccentFromColorGroup(property.colorGroup);
}

QPixmap PropertyPortfolioWidget::iconFor(const QString& assetName) const
{
    const auto it = iconCache.constFind(assetName);
    if (it != iconCache.constEnd()) {
        return it.value();
    }

    QPixmap icon;
    const QString imageDir = MonopolyUi::findImageDirectory();
    if (!imageDir.isEmpty()) {
        icon.load(imageDir + '/' + assetName);
    }

    iconCache.insert(assetName, icon);
    return icon;
}

void PropertyPortfolioWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    slotVisuals.clear();

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const QRectF content = rect().adjusted(4, 2, -4, -2);
    const qreal slotHeight = qBound<qreal>(14.0, content.height() * 0.098, 18.0);
    const qreal slotWidth = slotHeight * 0.74;
    const qreal compactWidth = slotWidth * 0.74;
    const qreal gap = qMax<qreal>(3.0, content.width() * 0.013);
    const qreal shadowOffset = 1.5;

    for (const GroupLayout& layout : kLayouts) {
        QVector<PortfolioPropertyView> groupEntries;
        for (const PortfolioPropertyView& entry : portfolio) {
            if (groupKindFor(entry) == layout.kind) {
                groupEntries.append(entry);
            }
        }

        if (groupEntries.isEmpty()) {
            continue;
        }

        std::sort(groupEntries.begin(), groupEntries.end(), [](const PortfolioPropertyView& left, const PortfolioPropertyView& right) {
            return left.propertyId < right.propertyId;
        });

        const QColor accent = accentColorFor(groupEntries.front());
        const qreal currentSlotWidth = layout.compact ? compactWidth : slotWidth;
        const qreal currentGap = layout.compact ? qMax<qreal>(2.4, gap * 0.58) : gap;
        const qreal x = content.left() + content.width() * layout.xRatio;
        const qreal y = content.top() + content.height() * layout.yRatio;

        bool ownsEverySlot = true;
        for (const PortfolioPropertyView& entry : groupEntries) {
            if (!entry.owned) {
                ownsEverySlot = false;
                break;
            }
        }

        const qreal frameWidth = layout.slotCount * currentSlotWidth + (layout.slotCount - 1) * currentGap;
        const QRectF frameRect(x - 6, y - 6, frameWidth + 12, slotHeight + 12);
        if (ownsEverySlot && groupEntries.size() == layout.slotCount && layout.kind != GroupLayout::Kind::Railroads && layout.kind != GroupLayout::Kind::Utilities) {
            painter.save();
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(143, 168, 216, 28));
            painter.drawRoundedRect(frameRect.translated(shadowOffset, shadowOffset), 7, 7);
            painter.setBrush(frameFill(accent));
            painter.setPen(QPen(accent, 2.4));
            painter.drawRoundedRect(frameRect, 7, 7);
            painter.restore();
        }

        for (int index = 0; index < groupEntries.size(); ++index) {
            const QRectF slotRect(
                x + index * (currentSlotWidth + currentGap),
                y,
                currentSlotWidth,
                slotHeight
            );

            SlotVisual visual;
            visual.rect = slotRect;
            visual.property = groupEntries[index];
            visual.accentColor = accentColorFor(groupEntries[index]);
            visual.selected = groupEntries[index].propertyId == selectedPropertyId;

            slotVisuals.append(visual);

            if (groupEntries[index].propertyType == PropertyType::STREET) {
                drawStreetSlot(painter, visual);
            } else {
                drawSpecialSlot(painter, visual);
            }
        }
    }
}

void PropertyPortfolioWidget::mousePressEvent(QMouseEvent *event)
{
    for (const SlotVisual& visual : slotVisuals) {
        if (!visual.rect.contains(event->pos())) {
            continue;
        }

        emit propertyActivated(visual.property.propertyId);
        return;
    }

    QWidget::mousePressEvent(event);
}

void PropertyPortfolioWidget::drawStreetSlot(QPainter& painter, const SlotVisual& visual) const
{
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing);

    const QRectF shadowRect = visual.rect.translated(1.6, 2.2);
    const QColor stripColor = visual.accentColor;

    if (visual.property.owned) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(143, 168, 216, 38));
        painter.drawRoundedRect(shadowRect, 3, 3);

        painter.setPen(QPen(QColor(176, 188, 198), 1.2));
        painter.setBrush(Qt::white);
        painter.drawRoundedRect(visual.rect, 3, 3);

        const QRectF stripRect = QRectF(visual.rect.left(), visual.rect.top(), visual.rect.width(), visual.rect.height() * 0.34);
        painter.fillRect(stripRect.adjusted(0.5, 0.5, -0.5, 0), stripColor);

        if (visual.property.mortgaged) {
            painter.setBrush(QColor(19, 28, 38, 110));
            painter.setPen(Qt::NoPen);
            painter.drawRoundedRect(visual.rect, 3, 3);

            painter.setPen(QPen(Qt::white, 2.0));
            painter.drawLine(visual.rect.bottomLeft() + QPointF(4, -4), visual.rect.topRight() + QPointF(-4, 4));

            QFont mortgageFont(QStringLiteral("Arial"), qMax(8, int(visual.rect.height() * 0.22)), QFont::Black);
            painter.setFont(mortgageFont);
            painter.drawText(visual.rect, Qt::AlignCenter, QStringLiteral("M"));
        }

        if (visual.property.buildingLevel > 0) {
            const QRectF ribbonRect(visual.rect.left() + 2, visual.rect.top() + 2, visual.rect.width() * 0.35, visual.rect.height() * 0.24);
            painter.setBrush(QColor(255, 206, 43));
            painter.setPen(Qt::NoPen);

            QPainterPath ribbon;
            ribbon.moveTo(ribbonRect.left(), ribbonRect.top());
            ribbon.lineTo(ribbonRect.right(), ribbonRect.top());
            ribbon.lineTo(ribbonRect.right(), ribbonRect.bottom() - 3);
            ribbon.lineTo(ribbonRect.left() + ribbonRect.width() * 0.72, ribbonRect.bottom() - 3);
            ribbon.lineTo(ribbonRect.left() + ribbonRect.width() * 0.52, ribbonRect.bottom() + 6);
            ribbon.lineTo(ribbonRect.left() + ribbonRect.width() * 0.52, ribbonRect.bottom() - 3);
            ribbon.lineTo(ribbonRect.left(), ribbonRect.bottom() - 3);
            ribbon.closeSubpath();
            painter.drawPath(ribbon);

            QFont buildFont(QStringLiteral("Arial"), qMax(7, int(visual.rect.height() * 0.17)), QFont::Black);
            painter.setFont(buildFont);
            painter.setPen(QColor(66, 47, 0));
            const QString badge = visual.property.buildingLevel >= 5
                ? QStringLiteral("H")
                : QString::number(visual.property.buildingLevel);
            painter.drawText(ribbonRect.adjusted(0, 0, -2, 0), Qt::AlignCenter, badge);
        }
    } else {
        painter.setPen(QPen(QColor(stripColor.red(), stripColor.green(), stripColor.blue(), 132), 1.5));
        painter.setBrush(QColor(255, 255, 255, 18));
        painter.drawRoundedRect(visual.rect, 3, 3);

        const QRectF stripRect = QRectF(visual.rect.left(), visual.rect.top(), visual.rect.width(), visual.rect.height() * 0.34);
        painter.fillRect(stripRect.adjusted(0.5, 0.5, -0.5, 0), QColor(stripColor.red(), stripColor.green(), stripColor.blue(), 128));
    }

    if (visual.selected) {
        drawSelectionOutline(painter, visual.rect.adjusted(-2, -2, 2, 2));
    }

    painter.restore();
}

void PropertyPortfolioWidget::drawSpecialSlot(QPainter& painter, const SlotVisual& visual) const
{
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing);

    const QRectF shadowRect = visual.rect.translated(1.6, 2.2);
    const QColor accent = visual.accentColor;

    if (visual.property.owned) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(143, 168, 216, 34));
        painter.drawRoundedRect(shadowRect, 3, 3);

        painter.setPen(QPen(QColor(176, 188, 198), 1.2));
        painter.setBrush(Qt::white);
        painter.drawRoundedRect(visual.rect, 3, 3);
    } else {
        painter.setPen(QPen(QColor(accent.red(), accent.green(), accent.blue(), 126), 1.45));
        painter.setBrush(QColor(255, 255, 255, 18));
        painter.drawRoundedRect(visual.rect, 3, 3);
    }

    if (visual.property.propertyType == PropertyType::RAILROAD) {
        QFont railroadFont(QStringLiteral("Arial"), qMax(8, int(visual.rect.height() * 0.23)), QFont::Black);
        painter.setFont(railroadFont);
        painter.setPen(visual.property.owned
            ? QColor(25, 32, 40)
            : QColor(115, 144, 156, 120));
        if (visual.property.owned || visual.property.code == QStringLiteral("ATL")) {
            painter.drawText(visual.rect.adjusted(3, 3, -3, -3), Qt::AlignCenter, specialGlyph(visual.property));
        }
    }

    if (visual.selected) {
        drawSelectionOutline(painter, visual.rect.adjusted(-2, -2, 2, 2));
    }

    painter.restore();
}

void PropertyPortfolioWidget::drawSelectionOutline(QPainter& painter, const QRectF& rect) const
{
    painter.save();
    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(QColor(255, 245, 133), 3));
    painter.drawRoundedRect(rect, 4, 4);
    painter.setPen(QPen(QColor(30, 123, 218), 1.6));
    painter.drawRoundedRect(rect.adjusted(2, 2, -2, -2), 3, 3);
    painter.restore();
}
