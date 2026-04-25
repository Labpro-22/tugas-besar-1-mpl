#pragma once

#include <string>

class PropertyTile;
class StreetTile;
enum class ColorGroup;

namespace TextFormatter {
    std::string formatMoney(int value);
    std::string formatColorGroup(ColorGroup colorGroup);
    std::string formatPropertyCategory(const PropertyTile& property);
    std::string formatBuildingLevel(int buildingLevel);
    std::string formatBuildingLabel(const StreetTile& street);
}
