#include "views/BoardRenderer.hpp"

#include <iostream>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "core/Board.hpp"
#include "core/TurnManager.hpp"
#include "models/Enums.hpp"
#include "models/Player.hpp"
#include "models/tiles/PropertyTile.hpp"
#include "models/tiles/RailroadTile.hpp"
#include "models/tiles/StreetTile.hpp"
#include "models/tiles/Tile.hpp"
#include "models/tiles/UtilityTile.hpp"

namespace {

    namespace ANSI {
        const std::string RESET = "\033[0m";
        const std::string COKLAT = "\033[43m\033[30m";
        const std::string BIRU_MUDA = "\033[46m\033[30m";
        const std::string PINK = "\033[45m\033[97m";
        const std::string ORANGE = "\033[41m\033[97m";
        const std::string MERAH = "\033[101m\033[30m";
        const std::string KUNING = "\033[103m\033[30m";
        const std::string HIJAU = "\033[42m\033[30m";
        const std::string BIRU_TUA = "\033[44m\033[97m";
        const std::string UTILITAS = "\033[47m\033[30m";
        const std::string DEFAULT_BG = "\033[100m\033[97m";
    }

    const int CELL_WIDTH = 10;
    const int SIDE_SIZE = 11;
    const int JAIL_INDEX = 10;

    void initColorMap(std::map<ColorGroup, std::string>& colorMap) {
        if (!colorMap.empty()) {
            return;
        }

        colorMap[ColorGroup::COKLAT] = ANSI::COKLAT;
        colorMap[ColorGroup::BIRU_MUDA] = ANSI::BIRU_MUDA;
        colorMap[ColorGroup::MERAH_MUDA] = ANSI::PINK;
        colorMap[ColorGroup::ORANGE] = ANSI::ORANGE;
        colorMap[ColorGroup::MERAH] = ANSI::MERAH;
        colorMap[ColorGroup::KUNING] = ANSI::KUNING;
        colorMap[ColorGroup::HIJAU] = ANSI::HIJAU;
        colorMap[ColorGroup::BIRU_TUA] = ANSI::BIRU_TUA;
        colorMap[ColorGroup::ABU_ABU] = ANSI::UTILITAS;
        colorMap[ColorGroup::DEFAULT] = ANSI::DEFAULT_BG;
    }

    std::string categoryLabel(ColorGroup colorGroup) {
        switch (colorGroup) {
            case ColorGroup::COKLAT:
                return "CK";
            case ColorGroup::BIRU_MUDA:
                return "BM";
            case ColorGroup::MERAH_MUDA:
                return "PK";
            case ColorGroup::ORANGE:
                return "OR";
            case ColorGroup::MERAH:
                return "MR";
            case ColorGroup::KUNING:
                return "KN";
            case ColorGroup::HIJAU:
                return "HJ";
            case ColorGroup::BIRU_TUA:
                return "BT";
            case ColorGroup::ABU_ABU:
                return "AB";
            case ColorGroup::DEFAULT:
            default:
                return "DF";
        }
    }

    ColorGroup getTileColorGroup(const Tile* tile) {
        const StreetTile* street = dynamic_cast<const StreetTile*>(tile);
        if (street != nullptr) {
            return street->getColorGroup();
        }

        const UtilityTile* utility = dynamic_cast<const UtilityTile*>(tile);
        if (utility != nullptr) {
            return ColorGroup::ABU_ABU;
        }

        return ColorGroup::DEFAULT;
    }

    std::string padToWidth(const std::string& text, int width) {
        if (static_cast<int>(text.size()) >= width) {
            return text.substr(0, width);
        }

        return text + std::string(width - text.size(), ' ');
    }

    std::string buildPawnIndicators(int tileIndex,
                                    const std::vector<Player>& players,
                                    const TurnManager& turnManager) {
        std::string result;

        for (int i = 0; i < static_cast<int>(players.size()); ++i) {
            const Player& player = players[i];

            if (player.isBankrupt() || player.getPosition() != tileIndex) {
                continue;
            }

            if (player.isJailed()) {
                result += "IN:" + std::to_string(i + 1);
                continue;
            }

            if (tileIndex == JAIL_INDEX) {
                result += "V:" + std::to_string(i + 1);
                continue;
            }

            bool isCurrentPlayer = turnManager.getCurrentPlayer() != nullptr
                && turnManager.getCurrentPlayer()->getUsername() == player.getUsername();

            result += isCurrentPlayer
                ? "*" + std::to_string(i + 1) + "*"
                : "(" + std::to_string(i + 1) + ")";
        }

        return result;
    }

    std::string buildPropertyStatus(const Tile* tile, const std::vector<Player>& players) {
        const PropertyTile* property = dynamic_cast<const PropertyTile*>(tile);
        if (property == nullptr || property->getStatus() == PropertyStatus::BANK) {
            return "";
        }

        std::string ownerLabel;
        for (int i = 0; i < static_cast<int>(players.size()); ++i) {
            if (property->getOwner() != nullptr &&
                players[i].getUsername() == property->getOwner()->getUsername()) {
                ownerLabel = "P" + std::to_string(i + 1);
                break;
            }
        }

        if (property->getStatus() == PropertyStatus::MORTGAGED) {
            return ownerLabel + "[M]";
        }

        const StreetTile* street = dynamic_cast<const StreetTile*>(property);
        if (street == nullptr) {
            return ownerLabel;
        }

        int level = street->getBuildingLevel();
        if (level == 5) {
            return ownerLabel + " *";
        }

        if (level > 0) {
            return ownerLabel + " " + std::string(level, '^');
        }

        return ownerLabel;
    }

    std::string getAnsiCode(const std::map<ColorGroup, std::string>& colorMap, ColorGroup colorGroup) {
        std::map<ColorGroup, std::string>::const_iterator it = colorMap.find(colorGroup);
        if (it != colorMap.end()) {
            return it->second;
        }

        return ANSI::DEFAULT_BG;
    }
}  // namespace

std::string BoardRenderer::colorize(const std::string& text, const std::string& ansiCode) const {
    return ansiCode + text + ANSI::RESET;
}

std::string BoardRenderer::renderTileCell(const Tile* tile, const std::vector<Player>& players) const {
    ColorGroup colorGroup = getTileColorGroup(tile);
    std::string ansiCode = getAnsiCode(colorMap, colorGroup);

    std::string line1 = padToWidth("[" + categoryLabel(colorGroup) + "] " + tile->getCode(), CELL_WIDTH);
    std::string line2 = padToWidth(buildPropertyStatus(tile, players), CELL_WIDTH);

    return colorize(line1, ansiCode) + "\n" + colorize(line2, ansiCode);
}

void BoardRenderer::renderLegend(const std::vector<Player>& players) const {
    std::cout << " ---------------------------------- \n";
    std::cout << " LEGENDA KEPEMILIKAN & STATUS\n";

    for (int i = 0; i < static_cast<int>(players.size()); ++i) {
        const Player& player = players[i];
        std::string status;

        if (player.isBankrupt()) {
            status = "[BANGKRUT]";
        } else if (player.isJailed()) {
            status = "[PENJARA]";
        }

        std::cout << " P" << (i + 1) << " : " << player.getUsername()
                  << " (M" << player.getBalance() << ") " << status << "\n";
    }

    std::cout << " ^ : Rumah Level 1\n";
    std::cout << " ^^ : Rumah Level 2\n";
    std::cout << " ^^^ : Rumah Level 3\n";
    std::cout << " ^^^^ : Rumah Level 4\n";
    std::cout << " * : Hotel (Maksimal)\n";
    std::cout << " (N) : Bidak Pemain N\n";
    std::cout << " IN:N : Pemain N di Penjara\n";
    std::cout << " V:N : Pemain N Hanya Mampir\n";
    std::cout << " [M] : Properti Digadaikan\n";
    std::cout << " ---------------------------------- \n";
    std::cout << " KODE WARNA:\n";
    std::cout << " " << colorize("[CK]=Coklat  ", ANSI::COKLAT)
              << " " << colorize("[MR]=Merah   ", ANSI::MERAH) << "\n";
    std::cout << " " << colorize("[BM]=Biru Muda", ANSI::BIRU_MUDA)
              << " " << colorize("[KN]=Kuning  ", ANSI::KUNING) << "\n";
    std::cout << " " << colorize("[PK]=Pink    ", ANSI::PINK)
              << " " << colorize("[HJ]=Hijau   ", ANSI::HIJAU) << "\n";
    std::cout << " " << colorize("[OR]=Orange  ", ANSI::ORANGE)
              << " " << colorize("[BT]=Biru Tua", ANSI::BIRU_TUA) << "\n";
    std::cout << " " << colorize("[DF]=Aksi    ", ANSI::DEFAULT_BG)
              << " " << colorize("[AB]=Utilitas", ANSI::UTILITAS) << "\n";
    std::cout << " ---------------------------------- \n";
}

void BoardRenderer::render(const Board& board,
                           const std::vector<Player>& players,
                           const TurnManager& turnManager) {
    initColorMap(colorMap);

    auto makeSeparator = []() -> std::string {
        std::string separator = "+";
        for (int i = 0; i < SIDE_SIZE; ++i) {
            separator += std::string(CELL_WIDTH, '-') + "+";
        }
        return separator;
    };

    auto renderCell = [&](int tileIndex) -> std::pair<std::string, std::string> {
        const Tile* tile = board.getTile(tileIndex);
        ColorGroup colorGroup = getTileColorGroup(tile);
        std::string ansiCode = getAnsiCode(colorMap, colorGroup);

        std::string line1 = padToWidth("[" + categoryLabel(colorGroup) + "] " + tile->getCode(), CELL_WIDTH);
        std::string propertyStatus = buildPropertyStatus(tile, players);
        std::string pawns = buildPawnIndicators(tileIndex, players, turnManager);
        std::string line2 = propertyStatus.empty() ? pawns : propertyStatus + " " + pawns;

        return std::make_pair(
            colorize(line1, ansiCode),
            colorize(padToWidth(line2, CELL_WIDTH), ansiCode)
        );
    };

    std::vector<int> topRow;
    std::vector<int> leftCol;
    std::vector<int> rightCol;
    std::vector<int> bottomRow;

    for (int i = 20; i <= 30; ++i) {
        topRow.push_back(i);
    }

    for (int i = 19; i >= 11; --i) {
        leftCol.push_back(i);
    }

    for (int i = 31; i <= 39; ++i) {
        rightCol.push_back(i);
    }
    rightCol.push_back(0);

    for (int i = 10; i >= 0; --i) {
        bottomRow.push_back(i);
    }

    std::string separator = makeSeparator();

    std::cout << separator << "\n";
    std::cout << "|";
    for (int tileIndex : topRow) {
        std::pair<std::string, std::string> cell = renderCell(tileIndex);
        std::cout << cell.first << "|";
    }
    std::cout << "\n";

    std::cout << "|";
    for (int tileIndex : topRow) {
        std::pair<std::string, std::string> cell = renderCell(tileIndex);
        std::cout << cell.second << "|";
    }
    std::cout << "\n";
    std::cout << separator << "\n";

    int innerWidth = (CELL_WIDTH + 1) * 9 - 1;
    int currentTurn = turnManager.getCurrentTurn();
    int maxTurn = turnManager.getMaxTurn();
    std::string activePlayer = "N/A";

    if (turnManager.getCurrentPlayer() != nullptr) {
        activePlayer = turnManager.getCurrentPlayer()->getUsername();
    }

    auto centerText = [innerWidth](const std::string& text) -> std::string {
        int totalPadding = innerWidth - static_cast<int>(text.size());
        if (totalPadding < 0) {
            return text.substr(0, innerWidth);
        }

        int leftPadding = totalPadding / 2;
        int rightPadding = totalPadding - leftPadding;
        return std::string(leftPadding, ' ') + text + std::string(rightPadding, ' ');
    };

    std::vector<std::string> panelLines;
    panelLines.push_back(std::string(innerWidth, ' '));
    panelLines.push_back(centerText("=================================="));
    panelLines.push_back(centerText("||        NIMONSPOLI            ||"));
    panelLines.push_back(centerText("=================================="));
    panelLines.push_back(std::string(innerWidth, ' '));

    std::string turnLine = "TURN " + std::to_string(currentTurn);
    if (maxTurn > 0) {
        turnLine += " / " + std::to_string(maxTurn);
    } else {
        turnLine += " (BANKRUPTCY MODE)";
    }

    panelLines.push_back(centerText(turnLine));
    panelLines.push_back(centerText("Giliran: " + activePlayer));
    panelLines.push_back(std::string(innerWidth, ' '));
    panelLines.push_back(std::string(innerWidth, ' '));

    for (int row = 0; row < 9; ++row) {
        std::pair<std::string, std::string> leftCell = renderCell(leftCol[row]);
        std::pair<std::string, std::string> rightCell = renderCell(rightCol[row]);
        std::string panel = panelLines[row];

        std::cout << "|" << leftCell.first << "|" << panel << "|" << rightCell.first << "|\n";
        std::cout << "|" << leftCell.second << "|" << std::string(innerWidth, ' ')
                  << "|" << rightCell.second << "|\n";
        std::cout << "+" << std::string(CELL_WIDTH, '-') << "+"
                  << std::string(innerWidth + 1, ' ')
                  << "+" << std::string(CELL_WIDTH, '-') << "+\n";
    }

    std::cout << separator << "\n";
    std::cout << "|";
    for (int tileIndex : bottomRow) {
        std::pair<std::string, std::string> cell = renderCell(tileIndex);
        std::cout << cell.first << "|";
    }
    std::cout << "\n";

    std::cout << "|";
    for (int tileIndex : bottomRow) {
        std::pair<std::string, std::string> cell = renderCell(tileIndex);
        std::cout << cell.second << "|";
    }
    std::cout << "\n";
    std::cout << separator << "\n";

    renderLegend(players);
}
