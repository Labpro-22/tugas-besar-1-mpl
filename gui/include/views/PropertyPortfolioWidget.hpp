#pragma once

#include <QColor>
#include <QHash>
#include <QPixmap>
#include <QRectF>
#include <QString>
#include <QVector>
#include <QWidget>

#include "models/Enums.hpp"

class QMouseEvent;
class QPaintEvent;

struct PortfolioPropertyView {
    int propertyId = 0;
    QString code;
    QString title;
    PropertyType propertyType = PropertyType::STREET;
    ColorGroup colorGroup = ColorGroup::DEFAULT;
    bool owned = false;
    bool mortgaged = false;
    int buildingLevel = 0;
};

class PropertyPortfolioWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PropertyPortfolioWidget(QWidget *parent = nullptr);

    void setPortfolio(const QVector<PortfolioPropertyView>& entries);
    void setSelectedPropertyId(int propertyId);

    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;

signals:
    void propertyActivated(int propertyId);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    struct SlotVisual {
        QRectF rect;
        PortfolioPropertyView property;
        QColor accentColor;
        bool selected = false;
    };

    QVector<PortfolioPropertyView> portfolio;
    QVector<SlotVisual> slotVisuals;
    mutable QHash<QString, QPixmap> iconCache;
    int selectedPropertyId = 0;

    QColor accentColorFor(const PortfolioPropertyView& property) const;
    QPixmap iconFor(const QString& assetName) const;
    void drawStreetSlot(QPainter& painter, const SlotVisual& visual) const;
    void drawSpecialSlot(QPainter& painter, const SlotVisual& visual) const;
    void drawSelectionOutline(QPainter& painter, const QRectF& rect) const;
};
