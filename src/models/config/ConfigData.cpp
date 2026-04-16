#include "models/config/ConfigData.hpp"

ConfigData::ConfigData() {}

ConfigData::ConfigData(
    const std::vector<PropertyConfig>& propertyConfigs,
    const std::map<int, int>& railroadRents,
    const std::map<int, int>& utilityMultipliers,
    const TaxConfig& taxConfig,
    const SpecialConfig& specialConfig,
    const MiscConfig& miscConfig
)
    : propertyConfigs(propertyConfigs),
      railroadRents(railroadRents),
      utilityMultipliers(utilityMultipliers),
      taxConfig(taxConfig),
      specialConfig(specialConfig),
      miscConfig(miscConfig) {}

const std::vector<PropertyConfig>& ConfigData::getPropertyConfigs() const {
    return propertyConfigs;
}

const std::map<int, int>& ConfigData::getRailroadRents() const {
    return railroadRents;
}

const std::map<int, int>& ConfigData::getUtilityMultipliers() const {
    return utilityMultipliers;
}

const TaxConfig& ConfigData::getTaxConfig() const {
    return taxConfig;
}

const SpecialConfig& ConfigData::getSpecialConfig() const {
    return specialConfig;
}

const MiscConfig& ConfigData::getMiscConfig() const {
    return miscConfig;
}
