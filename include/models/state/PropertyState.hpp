#pragma once

#include <string>

#include "models/Enums.hpp"

class PropertyState {
private:
    std::string code;
    PropertyType propertyType;
    std::string ownerUsername;
    PropertyStatus status;
    int festivalMultiplier;
    int festivalDuration;
    std::string buildingLevel;

public:
    PropertyState();
    PropertyState(
        const std::string& code,
        PropertyType propertyType,
        const std::string& ownerUsername,
        PropertyStatus status,
        int festivalMultiplier,
        int festivalDuration,
        const std::string& buildingLevel
    );

    const std::string& getCode() const;
    PropertyType getPropertyType() const;
    const std::string& getOwnerUsername() const;
    PropertyStatus getStatus() const;
    int getFestivalMultiplier() const;
    int getFestivalDuration() const;
    const std::string& getBuildingLevel() const;
};
