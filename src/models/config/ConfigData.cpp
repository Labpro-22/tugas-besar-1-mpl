#include "models/config/ConfigData.hpp"

ConfigData::ConfigData() {}

ConfigData::ConfigData(
    const std::vector<PropertyConfig>& propertyConfigs,
    const std::vector<ActionTileConfig>& actionTileConfigs,
    const std::map<int, int>& railroadRents,
    const std::map<int, int>& utilityMultipliers,
    const std::vector<std::string>& boardLayoutCodes,
    const std::string& sourcePath,
    const TaxConfig& taxConfig,
    const SpecialConfig& specialConfig,
    const MiscConfig& miscConfig
)
    : propertyConfigs(propertyConfigs),
      actionTileConfigs(actionTileConfigs),
      railroadRents(railroadRents),
      utilityMultipliers(utilityMultipliers),
      boardLayoutCodes(boardLayoutCodes),
      sourcePath(sourcePath),
      taxConfig(taxConfig),
      specialConfig(specialConfig),
      miscConfig(miscConfig) {}

const std::vector<PropertyConfig>& ConfigData::getPropertyConfigs() const {
    return propertyConfigs;
}

const std::vector<ActionTileConfig>& ConfigData::getActionTileConfigs() const {
    return actionTileConfigs;
}

const std::map<int, int>& ConfigData::getRailroadRents() const {
    return railroadRents;
}

const std::map<int, int>& ConfigData::getUtilityMultipliers() const {
    return utilityMultipliers;
}

const std::vector<std::string>& ConfigData::getBoardLayoutCodes() const {
    return boardLayoutCodes;
}

const std::string& ConfigData::getSourcePath() const {
    return sourcePath;
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
