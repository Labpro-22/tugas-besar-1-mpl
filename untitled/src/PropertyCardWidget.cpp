#include "PropertyCardWidget.hpp"

#include <QFont>
#include <QPaintEvent>
#include <QPainter>
#include <QRectF>
#include <QStringList>

#include "MonopolyUiShared.hpp"
#include "models/Enums.hpp"

namespace {

const QColor kPanelTop(245, 241, 228);
const QColor kPanelBottom(223, 214, 191);
const QColor kCardPaper(252, 251, 243);
const QColor kCardBorder(24, 24, 24);
const QColor kCardShadow(0, 0, 0, 55);
const QColor kBodyText(17, 21, 24);
const QColor kSecondaryText(54, 54, 54);

void drawHouseIcon(QPainter& painter, const QRectF& rect, const QColor& fill)
{
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    painter.setBrush(fill);

    const qreal roofHeight = rect.height() * 0.42;
    QPolygonF roof;
    roof << QPointF(rect.left(), rect.top() + roofHeight)
         << QPointF(rect.center().x(), rect.top())
         << QPointF(rect.right(), rect.top() + roofHeight);
    painter.drawPolygon(roof);

    QRectF body(rect.left() + rect.width() * 0.12, rect.top() + roofHeight, rect.width() * 0.76, rect.height() * 0.5);
    painter.drawRoundedRect(body, 1.2, 1.2);
    painter.restore();
}

QString normalizedPropertyName(const PropertyConfig& property)
{
    QString name = MonopolyUi::formatTileName(property.getName());
    name.replace('\n', ' ');
    return name;
}

QString rupiahLike(int amount)
{
    return MonopolyUi::formatCurrency(amount);
}

void drawCenteredRow(
    QPainter& painter,
    const QRectF& rowRect,
    const QString& label,
    const QString& value,
    bool emphasize
)
{
    QFont labelFont(QStringLiteral("Arial"), 10);
    labelFont.setWeight(emphasize ? QFont::Bold : QFont::DemiBold);
    painter.setFont(labelFont);
    painter.setPen(kBodyText);
    painter.drawText(rowRect.adjusted(10, 0, -10, 0), Qt::AlignLeft | Qt::AlignVCenter, label);

    QFont valueFont(QStringLiteral("Arial"), 10);
    valueFont.setWeight(emphasize ? QFont::Black : QFont::Bold);
    painter.setFont(valueFont);
    painter.drawText(rowRect.adjusted(10, 0, -10, 0), Qt::AlignRight | Qt::AlignVCenter, value);
}

}  // namespace

PropertyCardWidget::PropertyCardWidget(QWidget *parent)
    : QWidget(parent)
{
    setMinimumSize(330, 560);
}

void PropertyCardWidget::setConfigData(const ConfigData& configData)
{
    properties = configData.getPropertyConfigs();

    if (!properties.empty() && currentPropertyId == 0) {
        currentPropertyId = properties.front().getId();
    }

    update();
}

void PropertyCardWidget::setSelectedProperty(int propertyId)
{
    if (currentPropertyId == propertyId) {
        return;
    }

    currentPropertyId = propertyId;
    update();
}

int PropertyCardWidget::selectedPropertyId() const
{
    return currentPropertyId;
}

QSize PropertyCardWidget::minimumSizeHint() const
{
    return QSize(330, 560);
}

QSize PropertyCardWidget::sizeHint() const
{
    return QSize(380, 640);
}

const PropertyConfig* PropertyCardWidget::selectedProperty() const
{
    for (const PropertyConfig& property : properties) {
        if (property.getId() == currentPropertyId) {
            return &property;
        }
    }

    return properties.empty() ? nullptr : &properties.front();
}

void PropertyCardWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QLinearGradient panelGradient(rect().topLeft(), rect().bottomLeft());
    panelGradient.setColorAt(0.0, kPanelTop);
    panelGradient.setColorAt(1.0, kPanelBottom);
    painter.fillRect(rect(), panelGradient);

    const QRectF panel = rect().adjusted(18, 18, -18, -18);
    qreal cardWidth = qMin(panel.width(), panel.height() * (157.0 / 244.0));
    qreal cardHeight = cardWidth * (244.0 / 157.0);
    if (cardHeight > panel.height()) {
        cardHeight = panel.height();
        cardWidth = cardHeight * (157.0 / 244.0);
    }

    const QRectF cardRect(
        panel.center().x() - cardWidth / 2.0,
        panel.center().y() - cardHeight / 2.0,
        cardWidth,
        cardHeight
    );

    drawCardBase(painter, cardRect);

    const PropertyConfig* property = selectedProperty();
    if (!property) {
        drawEmptyState(painter, cardRect);
        return;
    }

    if (property->getPropertyType() == PropertyType::STREET) {
        drawPropertyCard(painter, cardRect, *property);
        return;
    }

    drawComingSoonCard(painter, cardRect, *property);
}

void PropertyCardWidget::drawCardBase(QPainter& painter, const QRectF& cardRect) const
{
    painter.save();
    painter.setPen(Qt::NoPen);
    painter.setBrush(kCardShadow);
    painter.drawRoundedRect(cardRect.translated(5, 7), 10, 10);

    painter.setPen(QPen(kCardBorder, 2.2));
    painter.setBrush(kCardPaper);
    painter.drawRoundedRect(cardRect, 8, 8);

    painter.setPen(QPen(QColor(30, 30, 30), 1.1));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(cardRect.adjusted(6, 6, -6, -6));
    painter.restore();
}

void PropertyCardWidget::drawPropertyCard(
    QPainter& painter,
    const QRectF& cardRect,
    const PropertyConfig& property
) const
{
    const QRectF content = cardRect.adjusted(10, 10, -10, -10);

    const qreal stripHeight = content.height() * 0.26;
    const qreal rentHeight = content.height() * 0.47;

    const QRectF stripRect(content.left(), content.top(), content.width(), stripHeight);
    const QRectF rentRect(content.left(), stripRect.bottom() + 3, content.width(), rentHeight);
    const QRectF footerRect(content.left(), rentRect.bottom() + 5, content.width(), content.bottom() - rentRect.bottom() - 5);

    const QColor groupColor = MonopolyUi::colorFromGroup(property.getColorGroup(), QColor(130, 130, 130));

    painter.save();
    painter.setPen(QPen(kCardBorder, 1.5));
    painter.setBrush(groupColor);
    painter.drawRect(stripRect);

    QFont codeFont(QStringLiteral("Arial"), 9);
    codeFont.setWeight(QFont::Bold);
    painter.setFont(codeFont);
    painter.setPen(groupColor.lightnessF() < 0.45 ? Qt::white : kBodyText);
    painter.drawText(stripRect.adjusted(0, 8, 0, -8), Qt::AlignHCenter | Qt::AlignTop, QStringLiteral("TITLE DEED"));

    QFont titleFont(QStringLiteral("Arial"), qMax(13, int(cardRect.width() * 0.085)));
    titleFont.setWeight(QFont::Black);
    painter.setFont(titleFont);
    painter.setPen(groupColor.lightnessF() < 0.45 ? Qt::white : kBodyText);
    painter.drawText(stripRect.adjusted(10, stripRect.height() * 0.30, -10, -10), Qt::AlignHCenter | Qt::AlignVCenter | Qt::TextWordWrap, normalizedPropertyName(property).toUpper());

    painter.setPen(QPen(kCardBorder, 1.0));
    painter.setBrush(QColor(255, 255, 255, 80));
    painter.drawRect(rentRect);

    const QStringList rentLabels = {
        QStringLiteral("Rent"),
        QStringLiteral("Rent with colour set"),
        QStringLiteral("Rent with"),
        QStringLiteral("Rent with"),
        QStringLiteral("Rent with"),
        QStringLiteral("Rent with")
    };

    const qreal rowHeight = rentRect.height() / rentLabels.size();
    for (int row = 0; row < rentLabels.size(); ++row) {
        const QRectF rowRect(rentRect.left(), rentRect.top() + row * rowHeight, rentRect.width(), rowHeight);
        if (row > 0) {
            painter.drawLine(rowRect.topLeft(), rowRect.topRight());
        }

        drawCenteredRow(
            painter,
            rowRect,
            row <= 1 ? rentLabels[row] : QString(),
            rupiahLike(property.getRentAtLevel(row)),
            row == 0
        );

        if (row >= 2) {
            const qreal iconSize = rowRect.height() * 0.55;
            const QRectF iconRect(
                rowRect.left() + rowRect.width() * 0.48,
                rowRect.center().y() - iconSize / 2.0,
                iconSize,
                iconSize
            );
            drawHouseIcon(painter, iconRect, row == 5 ? QColor(255, 88, 88) : QColor(40, 180, 99));
        }
    }

    const qreal footerRowHeight = footerRect.height() / 2.0;
    const QRectF houseCostRect(footerRect.left(), footerRect.top(), footerRect.width(), footerRowHeight);
    const QRectF hotelCostRect(footerRect.left(), houseCostRect.bottom(), footerRect.width(), footerRowHeight);

    painter.setPen(QPen(kCardBorder, 1.0));
    painter.drawLine(houseCostRect.bottomLeft(), houseCostRect.bottomRight());

    drawCenteredRow(painter, houseCostRect, QStringLiteral("Houses cost"), QStringLiteral("%1 each").arg(rupiahLike(property.getHouseCost())), false);
    drawCenteredRow(painter, hotelCostRect, QStringLiteral("Hotels cost"), QStringLiteral("%1 each").arg(rupiahLike(property.getHotelCost())), false);

    QFont extraFont(QStringLiteral("Arial"), 8);
    extraFont.setWeight(QFont::Normal);
    painter.setFont(extraFont);
    painter.setPen(kSecondaryText);
    painter.drawText(hotelCostRect.adjusted(0, hotelCostRect.height() * 0.45, -12, 0), Qt::AlignRight | Qt::AlignTop, QStringLiteral("(plus 4 houses)"));

    painter.restore();
}

void PropertyCardWidget::drawComingSoonCard(
    QPainter& painter,
    const QRectF& cardRect,
    const PropertyConfig& property
) const
{
    painter.save();

    QFont titleFont(QStringLiteral("Arial"), qMax(12, int(cardRect.width() * 0.08)));
    titleFont.setWeight(QFont::Black);
    painter.setFont(titleFont);
    painter.setPen(kBodyText);
    painter.drawText(
        cardRect.adjusted(20, 34, -20, -20),
        Qt::AlignHCenter | Qt::AlignTop,
        normalizedPropertyName(property)
    );

    QFont subFont(QStringLiteral("Arial"), 10);
    subFont.setWeight(QFont::DemiBold);
    painter.setFont(subFont);
    painter.setPen(kSecondaryText);
    painter.drawText(
        cardRect.adjusted(24, cardRect.height() * 0.40, -24, -24),
        Qt::AlignCenter | Qt::TextWordWrap,
        QStringLiteral("Rendering khusus Station dan Utility sedang disusun ulang.\nSaat ini fokus Property Card terlebih dahulu.")
    );

    painter.restore();
}

void PropertyCardWidget::drawEmptyState(QPainter& painter, const QRectF& cardRect) const
{
    painter.save();

    QFont titleFont(QStringLiteral("Arial"), qMax(14, int(cardRect.width() * 0.09)));
    titleFont.setWeight(QFont::Black);
    painter.setFont(titleFont);
    painter.setPen(kBodyText);
    painter.drawText(
        cardRect.adjusted(20, 38, -20, -20),
        Qt::AlignHCenter | Qt::AlignTop,
        QStringLiteral("Property Card")
    );

    QFont bodyFont(QStringLiteral("Arial"), 10);
    bodyFont.setWeight(QFont::Medium);
    painter.setFont(bodyFont);
    painter.setPen(kSecondaryText);
    painter.drawText(
        cardRect.adjusted(22, cardRect.height() * 0.36, -22, -22),
        Qt::AlignCenter | Qt::TextWordWrap,
        QStringLiteral("Belum ada data properti.\nPastikan file config/property.txt tersedia.")
    );

    painter.restore();
}
