#include "core/BoardFactory.hpp"

#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include "core/Board.hpp"
#include "models/config/ActionTileConfig.hpp"
#include "models/config/ConfigData.hpp"
#include "models/config/PropertyConfig.hpp"
#include "models/config/SpecialConfig.hpp"
#include "models/config/TaxConfig.hpp"
#include "models/tiles/CardTile.hpp"
#include "models/tiles/FestivalTile.hpp"
#include "models/tiles/FreeParkingTile.hpp"
#include "models/tiles/GoTile.hpp"
#include "models/tiles/GoToJailTile.hpp"
#include "models/tiles/JailTile.hpp"
#include "models/tiles/RailroadTile.hpp"
#include "models/tiles/StreetTile.hpp"
#include "models/tiles/TaxTile.hpp"
#include "models/tiles/Tile.hpp"
#include "models/tiles/UtilityTile.hpp"
#include "utils/exceptions/NimonspoliException.hpp"

namespace {
    int determineBoardTileCount(const ConfigData& configData) {
        int maxTileId = 0;

        for (const PropertyConfig& config : configData.getPropertyConfigs()) {
            if (config.getId() > maxTileId) {
                maxTileId = config.getId();
            }
        }

        for (const ActionTileConfig& config : configData.getActionTileConfigs()) {
            if (config.getId() > maxTileId) {
                maxTileId = config.getId();
            }
        }

        if (maxTileId <= 0) {
            throw ConfigException("", "tidak ada konfigurasi petak pada papan.");
        }

        return maxTileId;
    }

    Tile* createPropertyTile(
        const PropertyConfig& config,
        int index,
        const std::map<int, int>& railroadRents,
        const std::map<int, int>& utilityMultipliers
    ) {
        if (config.getPropertyType() == PropertyType::STREET) {
            int rentTable[6];
            for (int i = 0; i < 6; ++i) {
                rentTable[i] = config.getRentAtLevel(i);
            }

            return new StreetTile(
                index,
                config.getCode(),
                config.getName(),
                config.getColorGroup(),
                config.getBuyPrice(),
                config.getMortgageValue(),
                config.getHouseCost(),
                config.getHotelCost(),
                rentTable
            );
        }

        if (config.getPropertyType() == PropertyType::RAILROAD) {
            return new RailroadTile(
                index,
                config.getCode(),
                config.getName(),
                config.getMortgageValue(),
                railroadRents
            );
        }

        return new UtilityTile(
            index,
            config.getCode(),
            config.getName(),
            config.getMortgageValue(),
            utilityMultipliers
        );
    }

    Tile* createActionTile(const ActionTileConfig& actionConfig, int index, const ConfigData& configData) {
        const TaxConfig& taxConfig = configData.getTaxConfig();
        const SpecialConfig& specialConfig = configData.getSpecialConfig();
        const std::string& code = actionConfig.getCode();
        const std::string& name = actionConfig.getName();
        const std::string& tileType = actionConfig.getTileType();

        if (tileType == "KARTU") {
            if (code == "DNU") {
                return new CardTile(index, code, name, CardType::COMMUNITY_CHEST);
            }
            if (code == "KSP") {
                return new CardTile(index, code, name, CardType::CHANCE);
            }
        }

        if (tileType == "PAJAK") {
            if (code == "PPH") {
                return new TaxTile(
                    index,
                    code,
                    name,
                    TaxType::PPH,
                    taxConfig.getPphFlat(),
                    taxConfig.getPphPercentage()
                );
            }
            if (code == "PBM") {
                return new TaxTile(index, code, name, TaxType::PBM, taxConfig.getPbmFlat(), 0);
            }
        }

        if (tileType == "FESTIVAL") {
            return new FestivalTile(index, code, name);
        }

        if (tileType == "SPESIAL") {
            if (code == "GO") {
                return new GoTile(index, code, name, specialConfig.getGoSalary());
            }
            if (code == "PEN") {
                return new JailTile(index, code, name, specialConfig.getJailFine());
            }
            if (code == "BBP") {
                return new FreeParkingTile(index, code, name);
            }
            if (code == "PPJ") {
                return new GoToJailTile(index, code, name);
            }
        }

        throw ConfigException(
            "aksi.txt",
            "konfigurasi petak aksi tidak dikenali: " + code + " (" + tileType + ")");
    }

    void validateBoardTileCount(int boardTileCount, const std::string& source) {
        if (boardTileCount < 20 || boardTileCount > 60) {
            throw ConfigException(
                source,
                "jumlah petak papan harus berada pada rentang 20..60");
        }
        if (boardTileCount % 4 != 0) {
            throw ConfigException(
                source,
                "jumlah petak papan harus berbentuk 4n + 4, yaitu kelipatan 4");
        }
    }

    void validateSpecialTileCounts(const std::vector<Tile*>& boardTiles, const std::string& source) {
        int goCount = 0;
        int jailCount = 0;
        for (Tile* tile : boardTiles) {
            if (tile == nullptr) {
                continue;
            }
            if (tile->getCode() == "GO") {
                ++goCount;
            }
            if (tile->getCode() == "PEN") {
                ++jailCount;
            }
        }

        if (goCount != 1) {
            throw ConfigException(source, "papan harus memiliki tepat 1 petak GO");
        }
        if (jailCount != 1) {
            throw ConfigException(source, "papan harus memiliki tepat 1 petak Penjara (PEN)");
        }
    }

    std::vector<Tile*> buildDynamicBoardTiles(const ConfigData& configData) {
        const std::vector<std::string>& layoutCodes = configData.getBoardLayoutCodes();
        validateBoardTileCount(static_cast<int>(layoutCodes.size()), "papan.txt");

        std::map<std::string, const PropertyConfig*> propertyByCode;
        for (const PropertyConfig& config : configData.getPropertyConfigs()) {
            if (propertyByCode.find(config.getCode()) != propertyByCode.end()) {
                throw ConfigException("property.txt", "duplikasi kode properti: " + config.getCode());
            }
            propertyByCode[config.getCode()] = &config;
        }

        std::map<std::string, const ActionTileConfig*> actionByCode;
        for (const ActionTileConfig& config : configData.getActionTileConfigs()) {
            if (actionByCode.find(config.getCode()) == actionByCode.end()) {
                actionByCode[config.getCode()] = &config;
            }
        }

        std::set<std::string> usedProperties;
        std::vector<Tile*> boardTiles;
        boardTiles.reserve(layoutCodes.size());
        for (int index = 0; index < static_cast<int>(layoutCodes.size()); ++index) {
            const std::string& code = layoutCodes[static_cast<std::size_t>(index)];

            std::map<std::string, const PropertyConfig*>::const_iterator propertyIt = propertyByCode.find(code);
            if (propertyIt != propertyByCode.end()) {
                if (!usedProperties.insert(code).second) {
                    throw ConfigException(
                        "papan.txt",
                        "properti tidak boleh muncul lebih dari sekali pada papan dinamis: " + code);
                }
                boardTiles.push_back(createPropertyTile(
                    *propertyIt->second,
                    index,
                    configData.getRailroadRents(),
                    configData.getUtilityMultipliers()));
                continue;
            }

            std::map<std::string, const ActionTileConfig*>::const_iterator actionIt = actionByCode.find(code);
            if (actionIt != actionByCode.end()) {
                boardTiles.push_back(createActionTile(*actionIt->second, index, configData));
                continue;
            }

            throw ConfigException(
                "papan.txt",
                "kode petak tidak terdefinisi di property.txt atau aksi.txt: " + code);
        }

        validateSpecialTileCounts(boardTiles, "papan.txt");
        return boardTiles;
    }
}

void BoardFactory::build(Board& board, const ConfigData& configData) {
    if (!configData.getBoardLayoutCodes().empty()) {
        board.buildBoard(buildDynamicBoardTiles(configData));
        return;
    }

    const int boardTileCount = determineBoardTileCount(configData);
    validateBoardTileCount(boardTileCount, "property.txt/aksi.txt");

    std::vector<const PropertyConfig*> propertyById(boardTileCount + 1, nullptr);
    for (const PropertyConfig& config : configData.getPropertyConfigs()) {
        if (config.getId() < 1 || config.getId() > boardTileCount) {
            throw ConfigException(
                "property.txt",
                "ID properti berada di luar rentang papan: " + std::to_string(config.getId()));
        }
        if (propertyById[config.getId()] != nullptr) {
            throw ConfigException(
                "property.txt",
                "duplikasi konfigurasi properti pada petak " + std::to_string(config.getId()));
        }
        propertyById[config.getId()] = &config;
    }

    std::vector<const ActionTileConfig*> actionTileById(boardTileCount + 1, nullptr);
    for (const ActionTileConfig& config : configData.getActionTileConfigs()) {
        if (config.getId() < 1 || config.getId() > boardTileCount) {
            throw ConfigException(
                "aksi.txt",
                "ID petak aksi berada di luar rentang papan: " + std::to_string(config.getId()));
        }
        if (propertyById[config.getId()] != nullptr) {
            throw ConfigException(
                "property.txt/aksi.txt",
                "satu indeks papan tidak boleh berisi properti dan aksi sekaligus: " +
                    std::to_string(config.getId()));
        }
        if (actionTileById[config.getId()] != nullptr) {
            throw ConfigException(
                "aksi.txt",
                "duplikasi konfigurasi petak aksi pada petak " + std::to_string(config.getId()));
        }
        actionTileById[config.getId()] = &config;
    }

    std::vector<Tile*> boardTiles;
    boardTiles.reserve(boardTileCount);
    for (int index = 0; index < boardTileCount; ++index) {
        int tileId = index + 1;

        const ActionTileConfig* actionConfig = actionTileById[tileId];
        if (actionConfig != nullptr) {
            boardTiles.push_back(createActionTile(*actionConfig, index, configData));
            continue;
        }

        const PropertyConfig* propertyConfig = propertyById[tileId];
        if (propertyConfig == nullptr) {
            throw ConfigException(
                "",
                "konfigurasi petak tidak ditemukan untuk indeks " + std::to_string(tileId));
        }

        boardTiles.push_back(createPropertyTile(
            *propertyConfig,
            index,
            configData.getRailroadRents(),
            configData.getUtilityMultipliers()
        ));
    }

    validateSpecialTileCounts(boardTiles, "property.txt/aksi.txt");
    board.buildBoard(boardTiles);
}
