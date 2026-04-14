#pragma once

#include <array>
#include <string>

#include "models/Enums.hpp"

class PropertyConfig {
private:
    int id;
    std::string code;
    std::string name;
    PropertyType propertyType;
    ColorGroup colorGroup;
    int buyPrice;
    int mortgageValue;
    int houseCost;
    int hotelCost;
    std::array<int, 6> rentLevels;

public:
    PropertyConfig();
    PropertyConfig(
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
    );

    int getId() const;
    const std::string& getCode() const;
    const std::string& getName() const;
    PropertyType getPropertyType() const;
    ColorGroup getColorGroup() const;
    int getBuyPrice() const;
    int getMortgageValue() const;
    int getHouseCost() const;
    int getHotelCost() const;
    int getRentAtLevel(int level) const;
    const std::array<int, 6>& getRentLevels() const;
};
