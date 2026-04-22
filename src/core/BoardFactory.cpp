#include "core/BoardFactory.hpp"

#include <map>
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

namespace {
    Tile* createPropertyTile(
        const PropertyConfig& config,
        const std::map<int, int>& railroadRents,
        const std::map<int, int>& utilityMultipliers
    ) {
        int index = config.getId() - 1;

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

    Tile* createActionTile(const ActionTileConfig& actionConfig, const ConfigData& configData) {
        const TaxConfig& taxConfig = configData.getTaxConfig();
        const SpecialConfig& specialConfig = configData.getSpecialConfig();
        int index = actionConfig.getId() - 1;
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

        throw std::runtime_error(
            "Konfigurasi petak aksi tidak dikenali: " + code + " (" + tileType + ")");
    }
}

void BoardFactory::build(Board& board, const ConfigData& configData) {
    std::vector<const PropertyConfig*> propertyById(41, nullptr);
    for (const PropertyConfig& config : configData.getPropertyConfigs()) {
        if (config.getId() >= 1 && config.getId() <= 40) {
            if (propertyById[config.getId()] != nullptr) {
                throw std::runtime_error(
                    "Duplikasi konfigurasi properti pada petak "
                    + std::to_string(config.getId()));
            }
            propertyById[config.getId()] = &config;
        }
    }

    std::vector<const ActionTileConfig*> actionTileById(41, nullptr);
    for (const ActionTileConfig& config : configData.getActionTileConfigs()) {
        if (config.getId() >= 1 && config.getId() <= 40) {
            if (actionTileById[config.getId()] != nullptr) {
                throw std::runtime_error(
                    "Duplikasi konfigurasi petak aksi pada petak "
                    + std::to_string(config.getId()));
            }
            actionTileById[config.getId()] = &config;
        }
    }

    std::vector<Tile*> boardTiles;
    boardTiles.reserve(40);
    for (int index = 0; index < 40; ++index) {
        int tileId = index + 1;

        const ActionTileConfig* actionConfig = actionTileById[tileId];
        if (actionConfig != nullptr) {
            boardTiles.push_back(createActionTile(*actionConfig, configData));
            continue;
        }

        const PropertyConfig* propertyConfig = propertyById[tileId];
        if (propertyConfig == nullptr) {
            throw std::runtime_error(
                "Konfigurasi petak tidak ditemukan untuk indeks " + std::to_string(tileId));
        }

        boardTiles.push_back(createPropertyTile(
            *propertyConfig,
            configData.getRailroadRents(),
            configData.getUtilityMultipliers()
        ));
    }

    board.buildBoard(boardTiles);
}
