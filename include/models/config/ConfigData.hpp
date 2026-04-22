#pragma once

#include <map>
#include <vector>

#include "models/config/ActionTileConfig.hpp"
#include "models/config/MiscConfig.hpp"
#include "models/config/PropertyConfig.hpp"
#include "models/config/SpecialConfig.hpp"
#include "models/config/TaxConfig.hpp"

class ConfigData {
private:
    std::vector<PropertyConfig> propertyConfigs;
    std::vector<ActionTileConfig> actionTileConfigs;
    std::map<int, int> railroadRents;
    std::map<int, int> utilityMultipliers;
    TaxConfig taxConfig;
    SpecialConfig specialConfig;
    MiscConfig miscConfig;

public:
    ConfigData();
    ConfigData(
        const std::vector<PropertyConfig>& propertyConfigs,
        const std::vector<ActionTileConfig>& actionTileConfigs,
        const std::map<int, int>& railroadRents,
        const std::map<int, int>& utilityMultipliers,
        const TaxConfig& taxConfig,
        const SpecialConfig& specialConfig,
        const MiscConfig& miscConfig
    );

    const std::vector<PropertyConfig>& getPropertyConfigs() const;
    const std::vector<ActionTileConfig>& getActionTileConfigs() const;
    const std::map<int, int>& getRailroadRents() const;
    const std::map<int, int>& getUtilityMultipliers() const;
    const TaxConfig& getTaxConfig() const;
    const SpecialConfig& getSpecialConfig() const;
    const MiscConfig& getMiscConfig() const;
};
