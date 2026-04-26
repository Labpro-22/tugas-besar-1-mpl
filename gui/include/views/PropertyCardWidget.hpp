#pragma once

#include <QColor>
#include <QString>
#include <QWidget>

#include "models/config/ConfigData.hpp"

class QPaintEvent;
class QPainter;

class PropertyCardWidget : public QWidget
{
public:
    explicit PropertyCardWidget(QWidget *parent = nullptr);

    void setConfigData(const ConfigData& configData);
    void setSelectedProperty(int propertyId);
    void setOwnershipInfo(
        const QString& ownerName,
        const QColor& accentColor,
        bool mortgaged,
        int buildingLevel,
        int festivalMultiplier = 1,
        int festivalDuration = 0
    );
    int selectedPropertyId() const;

    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    const PropertyConfig* selectedProperty() const;
    void drawCardBase(QPainter& painter, const QRectF& cardRect) const;
    void drawPropertyCard(QPainter& painter, const QRectF& cardRect, const PropertyConfig& property) const;
    void drawRailroadCard(QPainter& painter, const QRectF& cardRect, const PropertyConfig& property) const;
    void drawUtilityCard(QPainter& painter, const QRectF& cardRect, const PropertyConfig& property) const;
    void drawOwnershipBadge(QPainter& painter, const QRectF& cardRect) const;
    void drawEmptyState(QPainter& painter, const QRectF& cardRect) const;

    ConfigData configData;
    QString ownerName = QStringLiteral("BANK");
    QColor ownerAccentColor;
    bool currentPropertyMortgaged = false;
    int currentBuildingLevel = 0;
    int currentFestivalMultiplier = 1;
    int currentFestivalDuration = 0;
    int currentPropertyId = 0;
};
