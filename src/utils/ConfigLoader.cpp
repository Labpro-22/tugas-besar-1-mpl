#include "utils/ConfigLoader.hpp"

#include "models/config/ConfigData.hpp"
#include "models/config/PropertyConfig.hpp"
#include "models/config/TaxConfig.hpp"
#include "models/config/SpecialConfig.hpp"
#include "models/config/MiscConfig.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <array>

PropertyType stringToType(const std::string& s) {
    if (s == "STREET") return PropertyType::STREET;
    if (s == "RAILROAD") return PropertyType::RAILROAD;
    return PropertyType::UTILITY;
}

ColorGroup stringToColor(const std::string& s) {
    if (s == "COKLAT") return ColorGroup::COKLAT;
    if (s == "BIRU_MUDA") return ColorGroup::BIRU_MUDA;
    if (s == "MERAH_MUDA") return ColorGroup::MERAH_MUDA;
    if (s == "ORANGE") return ColorGroup::ORANGE;
    if (s == "MERAH") return ColorGroup::MERAH;
    if (s == "KUNING") return ColorGroup::KUNING;
    if (s == "HIJAU") return ColorGroup::HIJAU;
    if (s == "BIRU_TUA") return ColorGroup::BIRU_TUA;
    if (s == "ABU_ABU") return ColorGroup::ABU_ABU;
    return ColorGroup::DEFAULT;
}

ConfigLoader::ConfigLoader(const std::string& configPath) : configPath(configPath) {
    if (!this->configPath.empty() && this->configPath.back() != '/' && this->configPath.back() != '\\') {
        this->configPath += '/';
    }
}

ConfigData ConfigLoader::loadAll() const {
    std::vector<PropertyConfig> properties = parsePropertyFile(configPath + "property.txt");
    std::map<int, int> railroads = parseRailroadFile(configPath + "railroad.txt");
    std::map<int, int> utilities = parseUtilityFile(configPath + "utility.txt");
    TaxConfig tax = parseTaxFile(configPath + "tax.txt");
    SpecialConfig special = parseSpecialFile(configPath + "special.txt");
    MiscConfig misc = parseMiscFile(configPath + "misc.txt");
 
    return ConfigData(properties, railroads, utilities, tax, special, misc);
}

std::vector<PropertyConfig>
ConfigLoader::parsePropertyFile(const std::string& path) const {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("ConfigLoader: cannot open '" + path + "'");
    }
    std::vector<PropertyConfig> result;
    std::string line;
    bool isHeader = true;
    
    while (std::getline(file,line)){
        if (line.empty() || line[0] == '#') continue;
        if (isHeader) { isHeader = false; continue; } 
        std::istringstream ss(line);
        
        int id, buyPrice, mortgageVal;
        std::string code, name, typeStr, colorStr;
        int houseCost    = 0;
        int hotelCost    = 0;
        std::array<int, 6> rentLevels = {0, 0, 0, 0, 0, 0};
 
        ss >> id >> code >> name >> typeStr >> colorStr >> buyPrice >> mortgageVal;
 
        if (ss.fail()) continue;
 
        if (typeStr == "STREET") {
            ss >> houseCost >> hotelCost;
            for (int i = 0; i < 6; ++i) {
                if (!(ss >> rentLevels[i])) break;
            }
        }
 
        result.emplace_back(
            id, code, name, 
            stringToType(typeStr),
            stringToColor(colorStr),
            buyPrice, mortgageVal, houseCost, hotelCost,
            rentLevels
        );
    }
 
    return result;
}

std::map<int, int>
ConfigLoader::parseRailroadFile(const std::string& path) const {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("ConfigLoader: cannot open '" + path + "'");
    }
 
    std::map<int, int> result;
    std::string line;
 
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
 
        std::istringstream ss(line);
        int count, rent;
        if (ss >> count >> rent) {
            result[count] = rent;
        }
    }
 
    return result;
}


std::map<int,int>
ConfigLoader::parseUtilityFile(const std::string& path) const {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("ConfigLoader: cannot open '" + path + "'");
    }
    std::map<int, int> result;
    std::string line;

    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
 
        std::istringstream ss(line);
        int count, multiplier;
        if (ss >> count >> multiplier) {
            result[count] = multiplier;
        }
    }
    return result;
}

TaxConfig ConfigLoader::parseTaxFile(const std::string& path) const{
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("ConfigLoader: cannot open '" + path + "'");
    }
 
    int pphFlat = 0, pphPct = 0, pbmFlat = 0;
    std::string line;
 
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
 
        std::istringstream ss(line);
        if (ss >> pphFlat >> pphPct >> pbmFlat) break;
    }
 
    return TaxConfig(pphFlat, pphPct, pbmFlat);
}

SpecialConfig ConfigLoader::parseSpecialFile(const std::string& path) const {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("ConfigLoader: cannot open '" + path + "'");
    }
 
    int goSalary = 0, jailFine = 0;
    std::string line;
 
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
 
        std::istringstream ss(line);
        if (ss >> goSalary >> jailFine) break;
    }
 
    return SpecialConfig(goSalary, jailFine);
}

MiscConfig ConfigLoader::parseMiscFile(const std::string& path) const {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("ConfigLoader: cannot open '" + path + "'");
    }
 
    int maxTurn = 0, initialBalance = 0;
    std::string line;
 
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
 
        std::istringstream ss(line);
        if (ss >> maxTurn >> initialBalance) break;
    }
 
    return MiscConfig(maxTurn, initialBalance);
}
