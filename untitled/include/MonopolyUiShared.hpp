#pragma once

#include <QColor>
#include <QString>

#include <string>

#include "models/Enums.hpp"

namespace MonopolyUi {

QString formatCurrency(int amount);
QString formatTileName(const std::string& rawName);

QString findConfigDirectory();
QString findImagesDirectory();
QString findPionDirectory();

QColor colorFromGroup(ColorGroup colorGroup, const QColor& fallback = QColor());

}  // namespace MonopolyUi
