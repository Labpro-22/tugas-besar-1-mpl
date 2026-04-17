#include "models/config/PropertyConfig.hpp"
#include <stdexcept>

PropertyConfig::PropertyConfig()
    : id(0),
      code(""),
      name(""),
      propertyType(PropertyType::STREET),
      colorGroup(ColorGroup::DEFAULT),
      buyPrice(0),
      mortgageValue(0),
      houseCost(0),
      hotelCost(0),
      rentLevels{0, 0, 0, 0, 0, 0} {}

PropertyConfig::PropertyConfig(
    int id,
    const std::string& code,
    const std::string& name,
    PropertyType propertyType,
    ColorGroup colorGroup,
    int buyPrice,
    int mortgageValue,
    int houseCost,
    int hotelCost,
    const std::array<int, 6>& rentLevels
)
    : id(id),
      code(code),
      name(name),
      propertyType(propertyType),
      colorGroup(colorGroup),
      buyPrice(buyPrice),
      mortgageValue(mortgageValue),
      houseCost(houseCost),
      hotelCost(hotelCost),
      rentLevels(rentLevels) {}

int PropertyConfig::getId() const {
    return id;
}

const std::string& PropertyConfig::getCode() const {
    return code;
}

const std::string& PropertyConfig::getName() const {
    return name;
}

PropertyType PropertyConfig::getPropertyType() const {
    return propertyType;
}

ColorGroup PropertyConfig::getColorGroup() const {
    return colorGroup;
}

int PropertyConfig::getBuyPrice() const {
    return buyPrice;
}

int PropertyConfig::getMortgageValue() const {
    return mortgageValue;
}

int PropertyConfig::getHouseCost() const {
    return houseCost;
}

int PropertyConfig::getHotelCost() const {
    return hotelCost;
}

int PropertyConfig::getRentAtLevel(int level) const {
    if (level < 0 || level > 5) {
        throw std::out_of_range("level harus berada pada rentang 0..5");
    }

    return rentLevels[level];
}

const std::array<int, 6>& PropertyConfig::getRentLevels() const {
    return rentLevels;
}
