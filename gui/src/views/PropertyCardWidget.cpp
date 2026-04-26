#include "views/PropertyCardWidget.hpp"

#include <array>

#include <QFont>
#include <QPaintEvent>
#include <QPainter>
#include <QPixmap>
#include <QRectF>
#include <QStringList>
#include <QtGui/QFontMetrics>

#include "utils/UiCommon.hpp"
#include "models/Enums.hpp"

namespace {

const QColor kPanelTop(241, 237, 226);
const QColor kPanelBottom(223, 215, 196);
const QColor kCardPaper(250, 249, 245);
const QColor kCardBorder(22, 22, 22);
const QColor kCardShadow(0, 0, 0, 46);
const QColor kBodyText(18, 18, 18);
const QColor kMutedText(56, 56, 56);
const QColor kHouseGreen(44, 182, 97);
const QColor kHotelRed(255, 90, 94);

QString propertyTitle(const PropertyConfig& property)
{
    return MonopolyUi::singleLineTileName(property.getName()).toUpper();
}

QPixmap uiAsset(const QString& assetName)
{
    QPixmap pixmap;
    const QString imageDir = MonopolyUi::findImageDirectory();
    if (!imageDir.isEmpty()) {
        pixmap.load(imageDir + '/' + assetName);
    }
    return pixmap;
}

void drawContainedPixmap(QPainter& painter, const QRectF& rect, const QPixmap& pixmap)
{
    if (pixmap.isNull() || rect.isEmpty()) {
        return;
    }

    const QSize scaled = pixmap.size().scaled(rect.size().toSize(), Qt::KeepAspectRatio);
    const QRect target(
        int(rect.center().x() - scaled.width() / 2.0),
        int(rect.center().y() - scaled.height() / 2.0),
        scaled.width(),
        scaled.height()
    );
    painter.drawPixmap(target, pixmap);
}

void drawHouseGlyph(QPainter& painter, const QRectF& rect, const QColor& fill, const QString& number = QString())
{
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    painter.setBrush(fill);

    QPolygonF roof;
    roof << QPointF(rect.left() + rect.width() * 0.08, rect.top() + rect.height() * 0.45)
         << QPointF(rect.center().x(), rect.top() + rect.height() * 0.10)
         << QPointF(rect.right() - rect.width() * 0.08, rect.top() + rect.height() * 0.45);
    painter.drawPolygon(roof);

    const QRectF body(
        rect.left() + rect.width() * 0.18,
        rect.top() + rect.height() * 0.42,
        rect.width() * 0.64,
        rect.height() * 0.42
    );
    painter.drawRect(body);

    if (!number.isEmpty()) {
        QFont numberFont(QStringLiteral("Trebuchet MS"), qMax(7, int(rect.height() * 0.46)), QFont::Black);
        painter.setFont(numberFont);
        painter.setPen(Qt::white);
        painter.drawText(body, Qt::AlignCenter, number);
    }

    painter.restore();
}

void drawLabelValueRow(
    QPainter& painter,
    const QRectF& rowRect,
    const QString& leftLabel,
    const QString& value,
    bool compact = false
)
{
    QFont labelFont(QStringLiteral("Trebuchet MS"), compact ? 11 : 12, QFont::DemiBold);
    painter.setFont(labelFont);
    painter.setPen(kBodyText);
    painter.drawText(rowRect.adjusted(0, 0, -6, 0), Qt::AlignLeft | Qt::AlignVCenter, leftLabel);

    QFont valueFont(QStringLiteral("Trebuchet MS"), compact ? 11 : 12, QFont::DemiBold);
    painter.setFont(valueFont);
    painter.drawText(rowRect.adjusted(6, 0, 0, 0), Qt::AlignRight | Qt::AlignVCenter, value);
}

qreal ownershipBadgeReserve(const QRectF& cardRect)
{
    return cardRect.height() * 0.15;
}

QFont fittedSingleLineFont(
    const QString& family,
    int desiredPointSize,
    int minimumPointSize,
    int weight,
    const QString& text,
    qreal availableWidth
)
{
    int pointSize = desiredPointSize;
    while (pointSize > minimumPointSize) {
        QFont font(family, pointSize, weight);
        if (QFontMetrics(font).horizontalAdvance(text) <= availableWidth) {
            return font;
        }
        --pointSize;
    }

    return QFont(family, minimumPointSize, weight);
}

}  // namespace

PropertyCardWidget::PropertyCardWidget(QWidget *parent)
    : QWidget(parent)
{
    setMinimumSize(280, 340);
}

void PropertyCardWidget::setConfigData(const ConfigData& newConfigData)
{
    configData = newConfigData;

    const std::vector<PropertyConfig>& properties = configData.getPropertyConfigs();
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

void PropertyCardWidget::setOwnershipInfo(
    const QString& newOwnerName,
    const QColor& accentColor,
    bool mortgaged,
    int buildingLevel
)
{
    ownerName = newOwnerName.isEmpty() ? QStringLiteral("BANK") : newOwnerName;
    ownerAccentColor = accentColor;
    currentPropertyMortgaged = mortgaged;
    currentBuildingLevel = buildingLevel;
    update();
}

int PropertyCardWidget::selectedPropertyId() const
{
    return currentPropertyId;
}

QSize PropertyCardWidget::minimumSizeHint() const
{
    return QSize(280, 340);
}

QSize PropertyCardWidget::sizeHint() const
{
    return QSize(340, 500);
}

const PropertyConfig* PropertyCardWidget::selectedProperty() const
{
    if (currentPropertyId == 0) {
        return nullptr;
    }

    const std::vector<PropertyConfig>& properties = configData.getPropertyConfigs();
    for (const PropertyConfig& property : properties) {
        if (property.getId() == currentPropertyId) {
            return &property;
        }
    }

    return nullptr;
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

    const QRectF panel = rect().adjusted(14, 14, -14, -14);
    qreal cardWidth = qMin(panel.width(), panel.height() * (0.67));
    qreal cardHeight = cardWidth * 1.46;
    if (cardHeight > panel.height()) {
        cardHeight = panel.height();
        cardWidth = cardHeight / 1.46;
    }

    const QRectF cardRect(
        panel.center().x() - cardWidth / 2.0,
        panel.center().y() - cardHeight / 2.0,
        cardWidth,
        cardHeight
    );

    drawCardBase(painter, cardRect);

    const PropertyConfig* property = selectedProperty();
    if (property == nullptr) {
        drawEmptyState(painter, cardRect);
        return;
    }

    switch (property->getPropertyType()) {
    case PropertyType::RAILROAD:
        drawRailroadCard(painter, cardRect, *property);
        break;
    case PropertyType::UTILITY:
        drawUtilityCard(painter, cardRect, *property);
        break;
    case PropertyType::STREET:
    default:
        drawPropertyCard(painter, cardRect, *property);
        break;
    }

    drawOwnershipBadge(painter, cardRect);
}

void PropertyCardWidget::drawCardBase(QPainter& painter, const QRectF& cardRect) const
{
    painter.save();
    painter.setPen(Qt::NoPen);
    painter.setBrush(kCardShadow);
    painter.drawRect(cardRect.translated(4, 5));

    painter.setPen(QPen(kCardBorder, 2.2));
    painter.setBrush(kCardPaper);
    painter.drawRect(cardRect);
    painter.restore();
}

void PropertyCardWidget::drawPropertyCard(
    QPainter& painter,
    const QRectF& cardRect,
    const PropertyConfig& property
) const
{
    painter.save();

    const QRectF inner = cardRect.adjusted(14, 14, -14, -14 - ownershipBadgeReserve(cardRect));
    const QRectF headerRect(inner.left(), inner.top(), inner.width(), inner.height() * 0.22);
    const QRectF bodyRect(inner.left(), headerRect.bottom() + 12, inner.width(), inner.height() * 0.50);
    const QRectF footerRect(inner.left(), bodyRect.bottom() + 10, inner.width(), inner.bottom() - bodyRect.bottom() - 10);

    const QColor groupColor = MonopolyUi::colorFromGroup(property.getColorGroup(), QColor(128, 128, 128));
    painter.setPen(QPen(kCardBorder, 1.8));
    painter.setBrush(groupColor);
    painter.drawRect(headerRect);

    QFont deedFont(QStringLiteral("Trebuchet MS"), qMax(10, int(cardRect.width() * 0.045)), QFont::Black);
    painter.setFont(deedFont);
    painter.setPen(groupColor.lightnessF() < 0.55 ? Qt::white : kBodyText);
    painter.drawText(headerRect.adjusted(0, 10, 0, -10), Qt::AlignHCenter | Qt::AlignTop, QStringLiteral("TITLE DEED"));

    QFont titleFont(QStringLiteral("Trebuchet MS"), qMax(15, int(cardRect.width() * 0.09)), QFont::Black);
    painter.setFont(titleFont);
    painter.drawText(headerRect.adjusted(12, 18, -12, -14), Qt::AlignCenter | Qt::TextWordWrap, propertyTitle(property));

    const qreal rowHeight = bodyRect.height() / 7.0;
    const QString valueBase = MonopolyUi::formatCurrency(property.getRentAtLevel(0));
    const QString valueSet = MonopolyUi::formatCurrency(property.getRentAtLevel(0) * 2);

    auto drawRentWithToken = [&](int rowIndex, const QString& value, int houses, bool hotel) {
        const QRectF rowRect(bodyRect.left(), bodyRect.top() + rowIndex * rowHeight, bodyRect.width(), rowHeight);
        const QString prefix = QStringLiteral("Rent with");
        QFont labelFont(QStringLiteral("Trebuchet MS"), qMax(10, int(cardRect.width() * 0.052)), QFont::DemiBold);
        painter.setFont(labelFont);
        painter.setPen(kBodyText);

        const qreal textY = rowRect.center().y();
        const QFontMetrics metrics(labelFont);
        const int prefixWidth = metrics.horizontalAdvance(prefix + ' ');
        const QRectF labelRect(rowRect.left(), rowRect.top(), rowRect.width() * 0.7, rowRect.height());
        painter.drawText(labelRect, Qt::AlignLeft | Qt::AlignVCenter, prefix);

        const QRectF iconRect(
            labelRect.left() + prefixWidth + 6,
            rowRect.center().y() - rowRect.height() * 0.24,
            rowRect.height() * 0.56,
            rowRect.height() * 0.56
        );
        if (hotel) {
            drawHouseGlyph(painter, iconRect, kHotelRed);
        } else {
            drawHouseGlyph(painter, iconRect, kHouseGreen, QString::number(houses));
        }

        QFont valueFont(QStringLiteral("Trebuchet MS"), qMax(11, int(cardRect.width() * 0.055)), QFont::DemiBold);
        painter.setFont(valueFont);
        painter.drawText(rowRect.adjusted(10, 0, 0, 0), Qt::AlignRight | Qt::AlignVCenter, value);
        Q_UNUSED(textY);
    };

    drawLabelValueRow(painter, QRectF(bodyRect.left(), bodyRect.top(), bodyRect.width(), rowHeight), QStringLiteral("Rent"), valueBase);
    drawLabelValueRow(painter, QRectF(bodyRect.left(), bodyRect.top() + rowHeight, bodyRect.width(), rowHeight), QStringLiteral("Rent with colour set"), valueSet);
    drawRentWithToken(2, MonopolyUi::formatCurrency(property.getRentAtLevel(1)), 1, false);
    drawRentWithToken(3, MonopolyUi::formatCurrency(property.getRentAtLevel(2)), 2, false);
    drawRentWithToken(4, MonopolyUi::formatCurrency(property.getRentAtLevel(3)), 3, false);
    drawRentWithToken(5, MonopolyUi::formatCurrency(property.getRentAtLevel(4)), 4, false);
    drawRentWithToken(6, MonopolyUi::formatCurrency(property.getRentAtLevel(5)), 0, true);

    painter.setPen(QPen(QColor(80, 80, 80), 1.2));
    painter.drawLine(
        QPointF(footerRect.left(), footerRect.top() + 4),
        QPointF(footerRect.right(), footerRect.top() + 4)
    );

    const QRectF houseRow(footerRect.left(), footerRect.top() + 18, footerRect.width(), footerRect.height() * 0.25);
    const QRectF hotelRow(footerRect.left(), houseRow.bottom() + 8, footerRect.width(), footerRect.height() * 0.25);
    drawLabelValueRow(
        painter,
        houseRow,
        QStringLiteral("Houses cost"),
        QStringLiteral("%1 each").arg(MonopolyUi::formatCurrency(property.getHouseCost()))
    );
    drawLabelValueRow(
        painter,
        hotelRow,
        QStringLiteral("Hotels cost"),
        QStringLiteral("%1 each").arg(MonopolyUi::formatCurrency(property.getHotelCost()))
    );

    QFont noteFont(QStringLiteral("Trebuchet MS"), qMax(8, int(cardRect.width() * 0.035)), QFont::Normal);
    painter.setFont(noteFont);
    painter.setPen(kMutedText);
    painter.drawText(
        footerRect.adjusted(0, hotelRow.bottom() - footerRect.top() + 2, -4, 0),
        Qt::AlignRight | Qt::AlignTop,
        QStringLiteral("(plus 4 houses)")
    );

    painter.restore();
}

void PropertyCardWidget::drawRailroadCard(
    QPainter& painter,
    const QRectF& cardRect,
    const PropertyConfig& property
) const
{
    painter.save();

    const QRectF inner = cardRect.adjusted(14, 16, -14, -16 - ownershipBadgeReserve(cardRect));
    const QRectF iconRect(
        inner.left() + inner.width() * 0.18,
        inner.top() + inner.height() * 0.03,
        inner.width() * 0.64,
        inner.height() * 0.18
    );
    const QRectF titleRect(inner.left(), iconRect.bottom() + 8, inner.width(), 40);
    const QRectF bodyRect(inner.left(), titleRect.bottom() + 10, inner.width(), inner.bottom() - titleRect.bottom() - 10);

    drawContainedPixmap(painter, iconRect, uiAsset(QStringLiteral("railroad_icon.png")));

    QFont titleFont(QStringLiteral("Trebuchet MS"), qMax(14, int(cardRect.width() * 0.075)), QFont::Black);
    painter.setFont(titleFont);
    painter.setPen(kBodyText);
    painter.drawText(titleRect, Qt::AlignHCenter | Qt::AlignVCenter | Qt::TextWordWrap, propertyTitle(property));

    const auto& railroadRents = configData.getRailroadRents();
    const std::array<QString, 4> labels = {
        QStringLiteral("RENT"),
        QStringLiteral("If 2 Stations are owned"),
        QStringLiteral("If 3 Stations are owned"),
        QStringLiteral("If 4 Stations are owned")
    };
    const std::array<int, 4> values = {
        railroadRents.count(1) ? railroadRents.at(1) : 25,
        railroadRents.count(2) ? railroadRents.at(2) : 50,
        railroadRents.count(3) ? railroadRents.at(3) : 100,
        railroadRents.count(4) ? railroadRents.at(4) : 200
    };

    const qreal rowHeight = bodyRect.height() / labels.size();
    for (int i = 0; i < static_cast<int>(labels.size()); ++i) {
        const QRectF row(bodyRect.left(), bodyRect.top() + i * rowHeight, bodyRect.width(), rowHeight);
        drawLabelValueRow(painter, row, labels[i], MonopolyUi::formatCurrency(values[i]), true);
    }

    painter.restore();
}

void PropertyCardWidget::drawUtilityCard(
    QPainter& painter,
    const QRectF& cardRect,
    const PropertyConfig& property
) const
{
    painter.save();

    const QRectF inner = cardRect.adjusted(14, 18, -14, -18 - ownershipBadgeReserve(cardRect));
    const QRectF iconRect(
        inner.left() + inner.width() * 0.22,
        inner.top() + inner.height() * 0.01,
        inner.width() * 0.56,
        inner.height() * 0.18
    );
    const QRectF titleRect(inner.left(), iconRect.bottom() + 8, inner.width(), 44);
    const QRectF textRect(inner.left() + 8, titleRect.bottom() + 8, inner.width() - 16, inner.bottom() - titleRect.bottom() - 8);

    const QString assetName = property.getCode() == "PLN"
        ? QStringLiteral("electric_company_icon.png")
        : QStringLiteral("waterworks_icon.png");
    drawContainedPixmap(painter, iconRect, uiAsset(assetName));

    QFont titleFont(QStringLiteral("Trebuchet MS"), qMax(14, int(cardRect.width() * 0.075)), QFont::Black);
    painter.setFont(titleFont);
    painter.setPen(kBodyText);
    painter.drawText(titleRect, Qt::AlignHCenter | Qt::AlignVCenter | Qt::TextWordWrap, propertyTitle(property));

    const auto& utilityMultipliers = configData.getUtilityMultipliers();
    const int singleUtility = utilityMultipliers.count(1) ? utilityMultipliers.at(1) : 4;
    const int bothUtilities = utilityMultipliers.count(2) ? utilityMultipliers.at(2) : 10;

    QFont bodyFont(QStringLiteral("Trebuchet MS"), qMax(10, int(cardRect.width() * 0.048)), QFont::DemiBold);
    painter.setFont(bodyFont);
    painter.setPen(kBodyText);
    painter.drawText(
        textRect,
        Qt::AlignHCenter | Qt::AlignTop | Qt::TextWordWrap,
        QStringLiteral(
            "If one Utility is owned,\n"
            "rent is %1 times amount\n"
            "shown on dice.\n\n"
            "If both Utilities are owned,\n"
            "rent is %2 times amount\n"
            "shown on dice."
        ).arg(singleUtility).arg(bothUtilities)
    );

    painter.restore();
}

void PropertyCardWidget::drawOwnershipBadge(QPainter& painter, const QRectF& cardRect) const
{
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing);

    const QString ownerText = ownerName == QStringLiteral("BANK")
        ? QStringLiteral("OWNER: BANK")
        : QStringLiteral("OWNER: %1").arg(ownerName);

    QStringList statusParts;
    if (currentPropertyMortgaged) {
        statusParts.append(QStringLiteral("MORTGAGED"));
    }
    if (currentBuildingLevel >= 5) {
        statusParts.append(QStringLiteral("HOTEL"));
    } else if (currentBuildingLevel > 0) {
        statusParts.append(QStringLiteral("%1 HOUSE").arg(currentBuildingLevel));
    }

    const QString statusText = statusParts.isEmpty()
        ? QStringLiteral("ACTIVE")
        : statusParts.join(QStringLiteral(" | "));

    const QRectF badgeRect(
        cardRect.left() + cardRect.width() * 0.08,
        cardRect.bottom() - cardRect.height() * 0.13,
        cardRect.width() * 0.84,
        cardRect.height() * 0.088
    );

    QColor accent = ownerAccentColor.isValid() ? ownerAccentColor : QColor(34, 34, 34);
    if (ownerName == QStringLiteral("BANK")) {
        accent = QColor(70, 78, 88);
    }

    painter.setPen(QPen(kCardBorder, 1.4));
    painter.setBrush(QColor(255, 255, 255, 232));
    painter.drawRoundedRect(badgeRect, 5, 5);

    const QRectF stripe(badgeRect.left(), badgeRect.top(), badgeRect.width(), qMax<qreal>(5.0, badgeRect.height() * 0.18));
    painter.setPen(Qt::NoPen);
    painter.setBrush(accent);
    painter.drawRect(stripe);

    const QRectF ownerTextRect = badgeRect.adjusted(8, 5, -8, -badgeRect.height() * 0.42);
    const QRectF statusTextRect = badgeRect.adjusted(8, badgeRect.height() * 0.48, -8, -3);

    QFont ownerFont = fittedSingleLineFont(
        QStringLiteral("Trebuchet MS"),
        qMax(8, int(cardRect.width() * 0.034)),
        7,
        QFont::Black,
        ownerText,
        ownerTextRect.width());
    painter.setFont(ownerFont);
    painter.setPen(kBodyText);
    painter.drawText(
        ownerTextRect,
        Qt::AlignCenter,
        QFontMetrics(ownerFont).elidedText(ownerText, Qt::ElideRight, int(ownerTextRect.width())));

    QFont statusFont = fittedSingleLineFont(
        QStringLiteral("Trebuchet MS"),
        qMax(7, int(cardRect.width() * 0.028)),
        6,
        QFont::DemiBold,
        statusText,
        statusTextRect.width());
    painter.setFont(statusFont);
    painter.setPen(kMutedText);
    painter.drawText(
        statusTextRect,
        Qt::AlignCenter,
        QFontMetrics(statusFont).elidedText(statusText, Qt::ElideRight, int(statusTextRect.width())));

    painter.restore();
}

void PropertyCardWidget::drawEmptyState(QPainter& painter, const QRectF& cardRect) const
{
    painter.save();

    QFont titleFont(QStringLiteral("Trebuchet MS"), qMax(14, int(cardRect.width() * 0.09)), QFont::Black);
    painter.setFont(titleFont);
    painter.setPen(kBodyText);
    painter.drawText(
        cardRect.adjusted(20, 38, -20, -20),
        Qt::AlignHCenter | Qt::AlignTop,
        QStringLiteral("Property Card")
    );

    QFont bodyFont(QStringLiteral("Trebuchet MS"), 10, QFont::Medium);
    painter.setFont(bodyFont);
    painter.setPen(kMutedText);
    painter.drawText(
        cardRect.adjusted(22, cardRect.height() * 0.36, -22, -22),
        Qt::AlignCenter | Qt::TextWordWrap,
        QStringLiteral("Belum ada data properti.\nPastikan file config/property.txt tersedia.")
    );

    painter.restore();
}
