#include "utils/TextFormatter.hpp"

#include <string>

#include "models/Enums.hpp"
#include "models/tiles/PropertyTile.hpp"
#include "models/tiles/StreetTile.hpp"

namespace TextFormatter {
    std::string formatMoney(int value) {
        return std::string(value < 0 ? "-M" : "M") + std::to_string(value < 0 ? -value : value);
    }

    std::string formatColorGroup(ColorGroup colorGroup) {
        switch (colorGroup) {
            case ColorGroup::COKLAT:
                return "COKLAT";
            case ColorGroup::BIRU_MUDA:
                return "BIRU MUDA";
            case ColorGroup::MERAH_MUDA:
                return "MERAH MUDA";
            case ColorGroup::ORANGE:
                return "ORANGE";
            case ColorGroup::MERAH:
                return "MERAH";
            case ColorGroup::KUNING:
                return "KUNING";
            case ColorGroup::HIJAU:
                return "HIJAU";
            case ColorGroup::BIRU_TUA:
                return "BIRU TUA";
            case ColorGroup::ABU_ABU:
                return "ABU-ABU";
            case ColorGroup::DEFAULT:
            default:
                return "DEFAULT";
        }
    }

    std::string formatPropertyCategory(const PropertyTile& property) {
        if (property.getPropertyType() == PropertyType::STREET) {
            return formatColorGroup(property.getColorGroup());
        }

        if (property.getPropertyType() == PropertyType::RAILROAD) {
            return "STASIUN";
        }

        return "UTILITAS";
    }

    std::string formatBuildingLevel(int buildingLevel) {
        if (buildingLevel <= 0) {
            return "0 rumah";
        }
        if (buildingLevel == 5) {
            return "Hotel";
        }
        return std::to_string(buildingLevel) + " rumah";
    }

    std::string formatBuildingLabel(const StreetTile& street) {
        return formatBuildingLevel(street.getBuildingLevel());
    }
}
