#pragma once

#include <QColor>
#include <QString>

#include <string>

#include "models/Enums.hpp"
#include "models/config/PropertyConfig.hpp"

namespace MonopolyUi {

QString formatCurrency(int amount);
QString formatTileName(const std::string& rawName);
QString singleLineTileName(const std::string& rawName);

QString findConfigDirectory();
QString findImageDirectory();

QColor colorFromGroup(ColorGroup colorGroup, const QColor& fallback = QColor());
int displayBuyPrice(const PropertyConfig& property);

}  // namespace MonopolyUi
