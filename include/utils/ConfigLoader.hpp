#pragma once

#include <string>
#include <vector>
#include <map>

class ConfigData;
class ActionTileConfig;
class PropertyConfig;
class TaxConfig;
class SpecialConfig;
class MiscConfig;
enum class PropertyType;
enum class ColorGroup;

class ConfigLoader {
private:
    std::string configPath;

    std::vector<PropertyConfig> parsePropertyFile(const std::string& path) const;
    std::vector<ActionTileConfig> parseActionTileFile(const std::string& path) const;
    std::map<int, int> parseRailroadFile(const std::string& path) const;
    std::map<int, int> parseUtilityFile(const std::string& path) const;
    TaxConfig parseTaxFile(const std::string& path) const;
    SpecialConfig parseSpecialFile(const std::string& path) const;
    MiscConfig parseMiscFile(const std::string& path) const;
    static PropertyType stringToType(const std::string& value);
    static ColorGroup stringToColor(const std::string& value);

public:
    explicit ConfigLoader(const std::string& configPath);

    ConfigData loadAll() const;
};
