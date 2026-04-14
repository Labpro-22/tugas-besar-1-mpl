#pragma once

#include <map>
#include <vector>

#include "models/config/MiscConfig.hpp"
#include "models/config/PropertyConfig.hpp"
#include "models/config/SpecialConfig.hpp"
#include "models/config/TaxConfig.hpp"

class ConfigData {
private:
    std::vector<PropertyConfig> propertyConfigs;
    std::map<int, int> railroadRents;
    std::map<int, int> utilityMultipliers;
    TaxConfig taxConfig;
    SpecialConfig specialConfig;
    MiscConfig miscConfig;

public:
    ConfigData();
    ConfigData(
        const std::vector<PropertyConfig>& propertyConfigs,
        const std::map<int, int>& railroadRents,
        const std::map<int, int>& utilityMultipliers,
        const TaxConfig& taxConfig,
        const SpecialConfig& specialConfig,
        const MiscConfig& miscConfig
    );

    std::vector<PropertyConfig>& getPropertyConfigs();
    const std::vector<PropertyConfig>& getPropertyConfigs() const;
    std::map<int, int>& getRailroadRents();
    const std::map<int, int>& getRailroadRents() const;
    std::map<int, int>& getUtilityMultipliers();
    const std::map<int, int>& getUtilityMultipliers() const;
    TaxConfig& getTaxConfig();
    const TaxConfig& getTaxConfig() const;
    SpecialConfig& getSpecialConfig();
    const SpecialConfig& getSpecialConfig() const;
    MiscConfig& getMiscConfig();
    const MiscConfig& getMiscConfig() const;
};
