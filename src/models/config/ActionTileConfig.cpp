#include "models/config/ActionTileConfig.hpp"

ActionTileConfig::ActionTileConfig()
    : id(0),
      code(""),
      name(""),
      tileType(""),
      colorGroup(ColorGroup::DEFAULT) {}

ActionTileConfig::ActionTileConfig(
    int id,
    const std::string& code,
    const std::string& name,
    const std::string& tileType,
    ColorGroup colorGroup
)
    : id(id),
      code(code),
      name(name),
      tileType(tileType),
      colorGroup(colorGroup) {}

int ActionTileConfig::getId() const {
    return id;
}

const std::string& ActionTileConfig::getCode() const {
    return code;
}

const std::string& ActionTileConfig::getName() const {
    return name;
}

const std::string& ActionTileConfig::getTileType() const {
    return tileType;
}

ColorGroup ActionTileConfig::getColorGroup() const {
    return colorGroup;
}
