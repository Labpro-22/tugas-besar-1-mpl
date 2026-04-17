#include "models/state/PropertyState.hpp"

PropertyState::PropertyState()
    : code(""),
      propertyType(PropertyType::STREET),
      ownerUsername("BANK"),
      status(PropertyStatus::BANK),
      festivalMultiplier(1),
      festivalDuration(0),
      buildingLevel("0") {}

PropertyState::PropertyState(
    const std::string& code,
    PropertyType propertyType,
    const std::string& ownerUsername,
    PropertyStatus status,
    int festivalMultiplier,
    int festivalDuration,
    const std::string& buildingLevel
)
    : code(code),
      propertyType(propertyType),
      ownerUsername(ownerUsername),
      status(status),
      festivalMultiplier(festivalMultiplier),
      festivalDuration(festivalDuration),
      buildingLevel(buildingLevel) {}

const std::string& PropertyState::getCode() const {
    return code;
}

PropertyType PropertyState::getPropertyType() const {
    return propertyType;
}

const std::string& PropertyState::getOwnerUsername() const {
    return ownerUsername;
}

PropertyStatus PropertyState::getStatus() const {
    return status;
}

int PropertyState::getFestivalMultiplier() const {
    return festivalMultiplier;
}

int PropertyState::getFestivalDuration() const {
    return festivalDuration;
}

const std::string& PropertyState::getBuildingLevel() const {
    return buildingLevel;
}
