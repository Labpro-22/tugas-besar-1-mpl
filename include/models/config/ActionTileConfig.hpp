#pragma once

#include <string>

#include "models/Enums.hpp"

class ActionTileConfig {
private:
    int id;
    std::string code;
    std::string name;
    std::string tileType;
    ColorGroup colorGroup;

public:
    ActionTileConfig();
    ActionTileConfig(
        int id,
        const std::string& code,
        const std::string& name,
        const std::string& tileType,
        ColorGroup colorGroup
    );

    int getId() const;
    const std::string& getCode() const;
    const std::string& getName() const;
    const std::string& getTileType() const;
    ColorGroup getColorGroup() const;
};
