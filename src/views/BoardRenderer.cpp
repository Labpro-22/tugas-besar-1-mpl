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
#include "utils/TextFormatter.hpp"

namespace {

    namespace ANSI {
        const std::string RESET = "\033[0m";
        const std::string COKLAT = "\033[48;5;94m\033[97m";
        const std::string BIRU_MUDA = "\033[48;5;117m\033[30m";
        const std::string PINK = "\033[48;5;218m\033[30m";
        const std::string ORANGE = "\033[48;5;208m\033[30m";
        const std::string MERAH = "\033[48;5;196m\033[97m";
        const std::string KUNING = "\033[48;5;226m\033[30m";
        const std::string HIJAU = "\033[48;5;46m\033[30m";
        const std::string BIRU_TUA = "\033[48;5;19m\033[97m";
        const std::string UTILITAS = "\033[48;5;250m\033[30m";
        const std::string DEFAULT_BG = "\033[48;5;240m\033[97m";
    }

    const int CELL_WIDTH = 10;
    const int LEGEND_WIDTH = 38;

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

    ColorGroup getTileColorGroup(const Tile* tile) {
        if (tile == nullptr || tile->asPropertyTile() == nullptr) {
            return ColorGroup::DEFAULT;
        }

        const PropertyTile* property = tile->asPropertyTile();
        if (property->getPropertyType() == PropertyType::STREET) {
            return property->getColorGroup();
        }

        if (property->getPropertyType() == PropertyType::UTILITY) {
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

    std::string makeLegendBorder() {
        return "+" + std::string(LEGEND_WIDTH, '-') + "+";
    }

    std::string makeLegendRow(const std::string& text) {
        return "|" + padToWidth(text, LEGEND_WIDTH) + "|";
    }

    std::string buildPawnIndicators(int tileIndex,
                                    int jailIndex,
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

            if (tileIndex == jailIndex) {
                result += "V:" + std::to_string(i + 1);
                continue;
            }

            bool isCurrentPlayer = turnManager.getCurrentPlayer() != nullptr
                && turnManager.getCurrentPlayer()->getUsername() == player.getUsername();

            result += isCurrentPlayer
                ? "(" + std::to_string(i + 1) + ")"
                : std::to_string(i + 1);
        }

        return result;
    }

    std::string buildPropertyStatus(const Tile* tile, const std::vector<Player>& players) {
        if (tile == nullptr || tile->asPropertyTile() == nullptr) {
            return "";
        }

        const PropertyTile* property = tile->asPropertyTile();
        if (property->getStatus() == PropertyStatus::BANK) {
            return "";
        }

        std::string festivalMarker;
        if (property->getPropertyType() == PropertyType::STREET &&
            property->getFestivalDuration() > 0 &&
            property->getFestivalMultiplier() > 1) {
            festivalMarker = "F" + std::to_string(property->getFestivalMultiplier());
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
            return ownerLabel + "[M]" + festivalMarker;
        }

        if (property->getPropertyType() != PropertyType::STREET) {
            return ownerLabel;
        }

        int level = property->getBuildingLevel();
        if (level == 5) {
            return ownerLabel + "*" + festivalMarker;
        }

        if (level > 0) {
            return ownerLabel + std::string(level, '^') + festivalMarker;
        }

        return ownerLabel + festivalMarker;
    }

    std::string getAnsiCode(const std::map<ColorGroup, std::string>& colorMap, ColorGroup colorGroup) {
        std::map<ColorGroup, std::string>::const_iterator it = colorMap.find(colorGroup);
        if (it != colorMap.end()) {
            return it->second;
        }

        return ANSI::DEFAULT_BG;
    }

    std::string tileHeader(const Tile* tile) {
        if (tile == nullptr) {
            return "??";
        }

        std::string displayIndex = std::to_string(tile->getIndex() + 1);
        if (tile->getIndex() + 1 < 10) {
            displayIndex = "0" + displayIndex;
        }

        return displayIndex + " " + tile->getCode();
    }
}  // namespace

std::string BoardRenderer::colorize(const std::string& text, const std::string& ansiCode) const {
    return ansiCode + text + ANSI::RESET;
}

void BoardRenderer::renderLegend(const Board& board, const std::vector<Player>& players) const {
    (void)board;

    std::cout << "\n";
    std::cout << makeLegendBorder() << "\n";
    std::cout << makeLegendRow(" LEGENDA KEPEMILIKAN & STATUS") << "\n";
    std::cout << makeLegendBorder() << "\n";

    for (int i = 0; i < static_cast<int>(players.size()); ++i) {
        const Player& player = players[i];
        std::string status;

        if (player.isBankrupt()) {
            status = "[BANGKRUT]";
        } else if (player.isJailed()) {
            status = "[PENJARA]";
        }

        std::cout << " P" << (i + 1) << " : " << player.getUsername()
                  << " (" << TextFormatter::formatMoney(player.getBalance()) << ") " << status << "\n";
    }

    std::cout << "\n";
    std::cout << " ^/^^/^^^/^^^^ : Rumah level 1-4\n";
    std::cout << " *             : Hotel\n";
    std::cout << " NN KODE       : Nomor dan kode petak\n";
    std::cout << " PN            : Properti milik pemain N\n";
    std::cout << " FN            : Festival aktif xN pada properti\n";
    std::cout << " N / (N)       : Bidak pemain, (N) = giliran aktif\n";
    std::cout << " IN:N / V:N    : Di penjara / hanya mampir penjara\n";
    std::cout << " [M]           : Properti digadaikan\n";
    std::cout << "\n";
    std::cout << " KODE WARNA:\n";
    std::cout << " " << colorize("[CK]=Coklat    ", ANSI::COKLAT)
              << " " << colorize("[MR]=Merah    ", ANSI::MERAH) << "\n";
    std::cout << " " << colorize("[BM]=Biru Muda ", ANSI::BIRU_MUDA)
              << " " << colorize("[KN]=Kuning   ", ANSI::KUNING) << "\n";
    std::cout << " " << colorize("[PK]=Pink      ", ANSI::PINK)
              << " " << colorize("[HJ]=Hijau    ", ANSI::HIJAU) << "\n";
    std::cout << " " << colorize("[OR]=Orange    ", ANSI::ORANGE)
              << " " << colorize("[BT]=Biru Tua ", ANSI::BIRU_TUA) << "\n";
    std::cout << " " << colorize("[DF]=Aksi      ", ANSI::DEFAULT_BG)
              << " " << colorize("[AB]=Utilitas ", ANSI::UTILITAS) << "\n";
    std::cout << makeLegendBorder() << "\n";
}

void BoardRenderer::render(const Board& board,
                           const std::vector<Player>& players,
                           const TurnManager& turnManager) {
    initColorMap(colorMap);

    const int tileCount = board.getTileCount();
    const Tile* jailTile = board.getTile("PEN");
    const int jailIndex = jailTile == nullptr ? -1 : jailTile->getIndex();

    auto makeRowSeparator = [](int sideTileCount) -> std::string {
        std::string separator = "+";
        for (int column = 0; column <= sideTileCount; ++column) {
            separator += std::string(CELL_WIDTH, '-') + "+";
        }
        return separator;
    };

    auto makeMiddleSeparator = [](int sideTileCount) -> std::string {
        const int middleWidth = (sideTileCount - 1) * (CELL_WIDTH + 1) - 1;
        return "+" + std::string(CELL_WIDTH, '-') + "+"
            + std::string(middleWidth, ' ') + "+"
            + std::string(CELL_WIDTH, '-') + "+";
    };

    auto renderCell = [&](int tileIndex) -> std::pair<std::string, std::string> {
        const Tile* tile = board.getTile(tileIndex);
        ColorGroup colorGroup = getTileColorGroup(tile);
        std::string ansiCode = getAnsiCode(colorMap, colorGroup);

        std::string line1 = padToWidth(tileHeader(tile), CELL_WIDTH);
        std::string propertyStatus = buildPropertyStatus(tile, players);
        std::string pawns = buildPawnIndicators(tileIndex, jailIndex, players, turnManager);
        std::string line2 = propertyStatus.empty() ? pawns : propertyStatus + " " + pawns;

        return std::make_pair(
            colorize(line1, ansiCode),
            colorize(padToWidth(line2, CELL_WIDTH), ansiCode)
        );
    };

    int currentTurn = turnManager.getCurrentTurn();
    int maxTurn = turnManager.getMaxTurn();
    std::string activePlayer = "N/A";

    if (turnManager.getCurrentPlayer() != nullptr) {
        activePlayer = turnManager.getCurrentPlayer()->getUsername();
    }

    const int sideTileCount = tileCount / 4;
    const int middleWidth = (sideTileCount - 1) * (CELL_WIDTH + 1) - 1;
    const std::string fullSeparator = makeRowSeparator(sideTileCount);
    const std::string middleSeparator = makeMiddleSeparator(sideTileCount);

    auto centerText = [middleWidth](const std::string& text) -> std::string {
        int totalPadding = middleWidth - static_cast<int>(text.size());
        if (totalPadding < 0) {
            return text.substr(0, middleWidth);
        }

        int leftPadding = totalPadding / 2;
        int rightPadding = totalPadding - leftPadding;
        return std::string(leftPadding, ' ') + text + std::string(rightPadding, ' ');
    };

    std::vector<std::string> panelLines;
    panelLines.push_back(std::string(middleWidth, ' '));
    panelLines.push_back(centerText("=================================="));
    panelLines.push_back(centerText("||          NIMONSPOLI          ||"));
    panelLines.push_back(centerText("=================================="));
    panelLines.push_back(std::string(middleWidth, ' '));

    std::string turnLine = "TURN " + std::to_string(currentTurn);
    if (maxTurn > 0) {
        turnLine += " / " + std::to_string(maxTurn);
    } else {
        turnLine += " (BANKRUPTCY MODE)";
    }

    panelLines.push_back(centerText(turnLine));
    panelLines.push_back(centerText("Giliran: " + activePlayer));
    panelLines.push_back(centerText("Jumlah petak: " + std::to_string(tileCount)));
    panelLines.push_back(std::string(middleWidth, ' '));

    auto renderFullRow = [&](const std::vector<int>& tileIndices) {
        std::vector<std::pair<std::string, std::string> > cells;
        cells.reserve(tileIndices.size());

        for (int tileIndex : tileIndices) {
            cells.push_back(renderCell(tileIndex));
        }

        for (int line = 0; line < 2; ++line) {
            std::cout << "|";
            for (const std::pair<std::string, std::string>& cell : cells) {
                std::cout << (line == 0 ? cell.first : cell.second) << "|";
            }
            std::cout << "\n";
        }
    };

    std::vector<int> topRow;
    topRow.reserve(sideTileCount + 1);
    for (int column = 0; column <= sideTileCount; ++column) {
        topRow.push_back(2 * sideTileCount + column);
    }

    std::vector<int> bottomRow;
    bottomRow.reserve(sideTileCount + 1);
    for (int column = 0; column <= sideTileCount; ++column) {
        bottomRow.push_back(sideTileCount - column);
    }

    std::cout << fullSeparator << "\n";
    renderFullRow(topRow);
    std::cout << fullSeparator << "\n";

    for (int row = 1; row < sideTileCount; ++row) {
        const int leftIndex = 2 * sideTileCount - row;
        const int rightIndex = 3 * sideTileCount + row;
        std::pair<std::string, std::string> leftCell = renderCell(leftIndex);
        std::pair<std::string, std::string> rightCell = renderCell(rightIndex);
        const int panelStart = (row - 1) * 2;
        const std::string panelLine1 = panelStart < static_cast<int>(panelLines.size())
            ? padToWidth(panelLines[panelStart], middleWidth)
            : std::string(middleWidth, ' ');
        const std::string panelLine2 = panelStart + 1 < static_cast<int>(panelLines.size())
            ? padToWidth(panelLines[panelStart + 1], middleWidth)
            : std::string(middleWidth, ' ');

        std::cout << "|" << leftCell.first << "|" << panelLine1 << "|" << rightCell.first << "|\n";
        std::cout << "|" << leftCell.second << "|" << panelLine2
                  << "|" << rightCell.second << "|\n";
        std::cout << (row == sideTileCount - 1 ? fullSeparator : middleSeparator) << "\n";
    }

    renderFullRow(bottomRow);
    std::cout << fullSeparator << "\n";

    renderLegend(board, players);
}
