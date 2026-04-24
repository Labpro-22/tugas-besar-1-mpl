#include "utils/OutputFormatter.hpp"

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

#include "models/Enums.hpp"
#include "models/Player.hpp"
#include "models/cards/SkillCard.hpp"
#include "models/state/LogEntry.hpp"
#include "models/tiles/PropertyTile.hpp"
#include "models/tiles/StreetTile.hpp"

namespace {
    const int DEED_INNER_WIDTH = 48;
    const int MENU_INNER_WIDTH = 30;
    const int HELP_INNER_WIDTH = 61;

    std::string centerText(const std::string& text, int width) {
        if (static_cast<int>(text.size()) >= width) {
            return text.substr(0, width);
        }

        int totalPadding = width - static_cast<int>(text.size());
        int leftPadding = totalPadding / 2;
        int rightPadding = totalPadding - leftPadding;
        return std::string(leftPadding, ' ') + text + std::string(rightPadding, ' ');
    }

    std::string padRight(const std::string& text, int width) {
        if (static_cast<int>(text.size()) >= width) {
            return text.substr(0, width);
        }

        return text + std::string(width - static_cast<int>(text.size()), ' ');
    }

    std::string makeBorder(char fill = '=') {
        return "+" + std::string(DEED_INNER_WIDTH, fill) + "+";
    }

    std::string makeBorder(int width, char fill) {
        return "+" + std::string(width, fill) + "+";
    }

    std::string makeRow(const std::string& text) {
        return "|" + padRight(text, DEED_INNER_WIDTH) + "|";
    }

    std::string makeRow(const std::string& text, int width) {
        return "|" + padRight(text, width) + "|";
    }

    std::string makeCenteredRow(const std::string& text) {
        return "|" + centerText(text, DEED_INNER_WIDTH) + "|";
    }

    std::string makeKeyValueRow(const std::string& key, const std::string& value) {
        return makeRow(padRight(key, 18) + " : " + value);
    }

    std::string makePreviewKeyValueRow(const std::string& key, const std::string& value) {
        return makeRow(padRight(key, 14) + " : " + value);
    }

    std::string fitColumn(const std::string& text, int width) {
        return padRight(text, width);
    }

    std::string makeHelpCommandRow(const std::string& command, const std::string& description) {
        return makeRow(
            " " + fitColumn(command, 24) + " | " + fitColumn(description, 32),
            HELP_INNER_WIDTH
        );
    }

    std::string propertyStatusToString(PropertyStatus status) {
        switch (status) {
            case PropertyStatus::BANK:
                return "BANK";
            case PropertyStatus::OWNED:
                return "OWNED";
            case PropertyStatus::MORTGAGED:
                return "MORTGAGED";
            default:
                return "UNKNOWN";
        }
    }

    std::string buildStatusLabel(const PropertyTile* property) {
        std::string status = propertyStatusToString(property->getStatus());
        if (property->getOwner() == nullptr) {
            return status;
        }

        return status + " (" + property->getOwner()->getUsername() + ")";
    }

    std::string buildPropertyHeadline(const PropertyTile* property) {
        std::string category;
        if (property->getPropertyType() == PropertyType::STREET) {
            category = "[" + OutputFormatter::formatColorGroup(property->getColorGroup()) + "]";
        } else if (property->getPropertyType() == PropertyType::RAILROAD) {
            category = "[RAILROAD]";
        } else {
            category = "[UTILITY]";
        }

        return category + " " + property->getName() + " (" + property->getCode() + ")";
    }

    bool propertySorter(const PropertyTile* lhs, const PropertyTile* rhs) {
        if (lhs == nullptr || rhs == nullptr) {
            return lhs < rhs;
        }

        if (lhs->getPropertyType() == PropertyType::STREET &&
            rhs->getPropertyType() == PropertyType::STREET &&
            lhs->getColorGroup() != rhs->getColorGroup()) {
            return static_cast<int>(lhs->getColorGroup()) < static_cast<int>(rhs->getColorGroup());
        }

        return lhs->getIndex() < rhs->getIndex();
    }

    std::string propertyGroupLabel(const PropertyTile* property) {
        if (property == nullptr) {
            return "";
        }

        if (property->getPropertyType() == PropertyType::STREET) {
            return OutputFormatter::formatColorGroup(property->getColorGroup());
        }

        if (property->getPropertyType() == PropertyType::RAILROAD) {
            return "STASIUN";
        }

        return "UTILITAS";
    }

    std::string buildingLabel(const PropertyTile* property) {
        if (property == nullptr || property->getPropertyType() != PropertyType::STREET) {
            return "";
        }

        int level = property->getBuildingLevel();
        if (level <= 0) {
            return "";
        }

        return OutputFormatter::formatBuildingLevel(level);
    }

    std::string propertyListStatus(const PropertyTile* property) {
        if (property == nullptr) {
            return "UNKNOWN";
        }

        std::string status = propertyStatusToString(property->getStatus());
        if (property->getStatus() == PropertyStatus::MORTGAGED) {
            status += " [M]";
        }

        return status;
    }

    std::string festivalSummary(const PropertyTile* property) {
        if (property == nullptr || property->getFestivalDuration() <= 0) {
            return "";
        }

        return "Festival aktif x" + std::to_string(property->getFestivalMultiplier()) +
            ", sisa " + std::to_string(property->getFestivalDuration()) +
            " giliran, sewa kini " +
            OutputFormatter::formatMoney(
                property->getRentAtLevel(property->getBuildingLevel()) * property->getFestivalMultiplier()
            );
    }

    int propertyDisplayValue(const PropertyTile* property) {
        return property == nullptr ? 0 : property->getBuyPrice();
    }

    int propertyWealthValue(const PropertyTile* property) {
        if (property == nullptr) {
            return 0;
        }

        int total = property->getBuyPrice();
        if (property->getPropertyType() != PropertyType::STREET) {
            return total;
        }

        int level = property->getBuildingLevel();
        if (level >= 1 && level <= 4) {
            total += level * property->getHouseCost();
        } else if (level == 5) {
            total += (4 * property->getHouseCost()) + property->getHotelCost();
        }

        return total;
    }

    std::string formatPropertyRow(const PropertyTile* property) {
        std::ostringstream row;
        const std::string name = property->getName() + " (" + property->getCode() + ")";
        row << "  - " << std::left << std::setw(31) << name
            << std::setw(10) << buildingLabel(property)
            << std::setw(8) << OutputFormatter::formatMoney(propertyDisplayValue(property))
            << propertyListStatus(property);
        return row.str();
    }

    bool hasUsableSkillCard(const Player& player) {
        for (SkillCard* card : player.getHand()) {
            if (card != nullptr && (!player.isJailed() || card->canUseWhileJailed())) {
                return true;
            }
        }
        return false;
    }

    bool containsPlayer(const std::vector<Player*>& players, const Player* candidate) {
        return std::find(players.begin(), players.end(), candidate) != players.end();
    }
}

namespace OutputFormatter {
    std::string formatMoney(int value) {
        return std::string(value < 0 ? "-M" : "M") + std::to_string(value < 0 ? -value : value);
    }

    std::string formatColorGroup(ColorGroup colorGroup) {
        switch (colorGroup) {
            case ColorGroup::COKLAT:
                return "COKLAT";
            case ColorGroup::BIRU_MUDA:
                return "BIRU MUDA";
            case ColorGroup::MERAH_MUDA:
                return "MERAH MUDA";
            case ColorGroup::ORANGE:
                return "ORANGE";
            case ColorGroup::MERAH:
                return "MERAH";
            case ColorGroup::KUNING:
                return "KUNING";
            case ColorGroup::HIJAU:
                return "HIJAU";
            case ColorGroup::BIRU_TUA:
                return "BIRU TUA";
            case ColorGroup::ABU_ABU:
                return "ABU-ABU";
            case ColorGroup::DEFAULT:
            default:
                return "DEFAULT";
        }
    }

    std::string formatPropertyCategory(const PropertyTile& property) {
        if (property.getPropertyType() == PropertyType::STREET) {
            return formatColorGroup(property.getColorGroup());
        }

        if (property.getPropertyType() == PropertyType::RAILROAD) {
            return "STASIUN";
        }

        return "UTILITAS";
    }

    std::string formatBuildingLevel(int buildingLevel) {
        if (buildingLevel <= 0) {
            return "0 rumah";
        }
        if (buildingLevel == 5) {
            return "Hotel";
        }
        return std::to_string(buildingLevel) + " rumah";
    }

    std::string formatBuildingLabel(const StreetTile& street) {
        return formatBuildingLevel(street.getBuildingLevel());
    }

    std::vector<std::string> formatLogEntries(const std::vector<LogEntry>& entries) {
        std::vector<std::string> lines;
        std::size_t turnWidth = 0;
        std::size_t usernameWidth = 0;
        std::size_t actionWidth = 0;

        for (const LogEntry& entry : entries) {
            std::ostringstream turnLabel;
            turnLabel << "[Turn " << entry.getTurn() << "]";
            turnWidth = std::max(turnWidth, turnLabel.str().size());
            usernameWidth = std::max(usernameWidth, entry.getUsername().size());
            actionWidth = std::max(actionWidth, entry.getActionType().size());
        }

        for (const LogEntry& entry : entries) {
            std::ostringstream turnLabel;
            turnLabel << "[Turn " << entry.getTurn() << "]";

            std::ostringstream line;
            line << std::left
                 << std::setw(static_cast<int>(turnWidth)) << turnLabel.str() << " "
                 << std::setw(static_cast<int>(usernameWidth)) << entry.getUsername() << " | "
                 << std::setw(static_cast<int>(actionWidth)) << entry.getActionType() << " | "
                 << entry.getDetail();
            lines.push_back(line.str());
        }

        return lines;
    }

    std::vector<std::string> formatMainMenu() {
        return {
            makeBorder(MENU_INNER_WIDTH, '-'),
            makeRow(centerText("NIMONSPOLI", MENU_INNER_WIDTH), MENU_INNER_WIDTH),
            makeBorder(MENU_INNER_WIDTH, '-'),
            makeRow(" 1. Game Baru", MENU_INNER_WIDTH),
            makeRow(" 2. Muat Game", MENU_INNER_WIDTH),
            makeBorder(MENU_INNER_WIDTH, '-'),
            "Catatan load: MUAT <filename> dari folder data/"
        };
    }

    std::vector<std::string> formatLoadCommandPrompt() {
        return {
            "",
            "Masukkan command load sesuai spesifikasi.",
            "Contoh: MUAT game_sesi1.txt",
            "File akan dicari dari folder data/."
        };
    }

    std::vector<std::string> formatHelpCommands(const Player& player) {
        const bool canUseSkillCard = hasUsableSkillCard(player);
        std::vector<std::string> lines = {
            "",
            makeBorder(HELP_INNER_WIDTH, '-'),
            makeRow(" COMMAND TERSEDIA", HELP_INNER_WIDTH),
            makeBorder(HELP_INNER_WIDTH, '-'),
            makeHelpCommandRow("CETAK_PAPAN", "tampilkan papan"),
            makeHelpCommandRow("CETAK_AKTA KODE", "tampilkan akta properti"),
            makeHelpCommandRow("CETAK_PROPERTI", "tampilkan properti pemain"),
            makeHelpCommandRow("CETAK_LOG [n]", "tampilkan log transaksi"),
            makeHelpCommandRow("SIMPAN file", "simpan game ke folder data/")
        };

        if (player.isJailed()) {
            lines.push_back(makeHelpCommandRow("BAYAR_DENDA", "keluar dari penjara dengan denda"));
            if (!player.hasUsedSkillThisTurn() && canUseSkillCard) {
                lines.push_back(makeHelpCommandRow("GUNAKAN_KEMAMPUAN", "pakai kartu non-pergerakan"));
            }
            if (!player.hasRolledThisTurn() && player.getJailTurns() <= 3) {
                lines.push_back(makeHelpCommandRow("LEMPAR_DADU", "coba keluar penjara dengan double"));
                lines.push_back(makeHelpCommandRow("ATUR_DADU X Y", "set dadu untuk percobaan double"));
            }
        } else {
            if (!player.hasRolledThisTurn()) {
                lines.push_back(makeHelpCommandRow("LEMPAR_DADU", "lempar dadu"));
                lines.push_back(makeHelpCommandRow("ATUR_DADU X Y", "set nilai dadu manual"));
                if (!player.hasUsedSkillThisTurn() && canUseSkillCard) {
                    lines.push_back(makeHelpCommandRow("GUNAKAN_KEMAMPUAN", "pakai kartu skill"));
                }
            }
            lines.push_back(makeHelpCommandRow("GADAI / TEBUS / BANGUN", "kelola aset"));
        }

        lines.push_back(makeHelpCommandRow("HELP / KELUAR", "bantuan / keluar"));
        lines.push_back(makeBorder(HELP_INNER_WIDTH, '-'));
        return lines;
    }

    std::vector<std::string> formatSectionTitle(const std::string& title) {
        return {
            "",
            "============================================================",
            " " + title,
            "============================================================"
        };
    }

    std::vector<std::string> formatTurnSummary(const Player& player) {
        return {
            "Saldo     : " + formatMoney(player.getBalance()),
            "Posisi    : " + std::to_string(player.getPosition() + 1),
            "Properti  : " + std::to_string(player.getProperties().size()),
            "Kartu     : " + std::to_string(player.getHand().size())
        };
    }

    std::vector<std::string> formatDiceLanding(
        int die1,
        int die2,
        int total,
        const std::string& playerName,
        const std::string& tileName,
        const std::string& tileCode
    ) {
        return {
            "",
            "Hasil: " + std::to_string(die1) + " + " + std::to_string(die2) + " = " + std::to_string(total),
            "Memajukan Bidak " + playerName + " sebanyak " + std::to_string(total) + " petak...",
            "Bidak mendarat di: " + tileName + " (" + tileCode + ")"
        };
    }

    std::vector<std::string> formatWinnerSummary(
        const std::vector<Player*>& winners,
        const std::vector<Player>& players,
        bool maxTurnReached
    ) {
        std::vector<std::string> lines;

        if (maxTurnReached) {
            lines.push_back("Permainan selesai! (Batas giliran tercapai)");
            lines.push_back("");
            lines.push_back("Rekap pemain:");
            lines.push_back("");

            for (const Player& player : players) {
                lines.push_back(player.getUsername());
                lines.push_back("Uang      : " + formatMoney(player.getBalance()));
                lines.push_back("Properti  : " + std::to_string(player.getProperties().size()));
                lines.push_back("Kartu     : " + std::to_string(player.getHand().size()));
                if (player.isBankrupt()) {
                    lines.push_back("Status    : BANKRUPT");
                }
                lines.push_back("");
            }
        } else {
            lines.push_back("Permainan selesai! (Semua pemain kecuali satu bangkrut)");
            lines.push_back("");
            lines.push_back("Pemain tersisa:");
            for (Player* player : winners) {
                if (player != nullptr) {
                    lines.push_back("- " + player->getUsername());
                }
            }

            std::vector<Player*> remainingPlayers;
            for (const Player& player : players) {
                if (!player.isBankrupt()) {
                    remainingPlayers.push_back(const_cast<Player*>(&player));
                }
            }

            for (Player* player : remainingPlayers) {
                if (player == nullptr || containsPlayer(winners, player)) {
                    continue;
                }
                lines.push_back("- " + player->getUsername());
            }
            lines.push_back("");
        }

        if (winners.empty()) {
            lines.push_back("Pemenang: -");
            return lines;
        }

        if (winners.size() == 1 && winners.front() != nullptr) {
            lines.push_back("Pemenang: " + winners.front()->getUsername());
            return lines;
        }

        lines.push_back("Pemenang:");
        for (Player* player : winners) {
            if (player != nullptr) {
                lines.push_back("- " + player->getUsername());
            }
        }

        return lines;
    }

    std::vector<std::string> formatStreetPurchasePreview(const PropertyTile& tile, const StreetTile& street) {
        const std::string colorLabel = formatColorGroup(tile.getColorGroup());
        const std::string buyPrice = formatMoney(tile.getBuyPrice());
        const std::string rent0 = formatMoney(street.getRentAtLevel(0));
        const std::string rent1 = formatMoney(street.getRentAtLevel(1));
        const std::string rent2 = formatMoney(street.getRentAtLevel(2));
        const std::string rent3 = formatMoney(street.getRentAtLevel(3));
        const std::string rent4 = formatMoney(street.getRentAtLevel(4));
        const std::string rent5 = formatMoney(street.getRentAtLevel(5));
        const std::string headline = "[" + colorLabel + "] " + tile.getName() + " (" + tile.getCode() + ")";

        return {
            makeBorder('='),
            makeRow(headline),
            makePreviewKeyValueRow("Harga Beli", buyPrice),
            makePreviewKeyValueRow("Sewa dasar", rent0),
            makePreviewKeyValueRow("Sewa 1 rumah", rent1),
            makePreviewKeyValueRow("Sewa 2 rumah", rent2),
            makePreviewKeyValueRow("Sewa 3 rumah", rent3),
            makePreviewKeyValueRow("Sewa 4 rumah", rent4),
            makePreviewKeyValueRow("Sewa hotel", rent5),
            makeBorder('=')
        };
    }

    std::vector<std::string> formatPropertyDeed(const PropertyTile* property) {
        if (property == nullptr) {
            return {"Properti tidak valid."};
        }

        std::vector<std::string> lines = {
            makeBorder('='),
            makeCenteredRow("AKTA KEPEMILIKAN"),
            makeCenteredRow(buildPropertyHeadline(property)),
            makeBorder('='),
            makeKeyValueRow("Harga Beli", formatMoney(property->getBuyPrice())),
            makeKeyValueRow("Nilai Gadai", formatMoney(property->getMortgageValue()))
        };

        if (property->getPropertyType() == PropertyType::STREET) {
            lines.push_back(makeBorder('-'));
            lines.push_back(makeKeyValueRow("Sewa (unimproved)", formatMoney(property->getRentAtLevel(0))));
            lines.push_back(makeKeyValueRow("Sewa (1 rumah)", formatMoney(property->getRentAtLevel(1))));
            lines.push_back(makeKeyValueRow("Sewa (2 rumah)", formatMoney(property->getRentAtLevel(2))));
            lines.push_back(makeKeyValueRow("Sewa (3 rumah)", formatMoney(property->getRentAtLevel(3))));
            lines.push_back(makeKeyValueRow("Sewa (4 rumah)", formatMoney(property->getRentAtLevel(4))));
            lines.push_back(makeKeyValueRow("Sewa (hotel)", formatMoney(property->getRentAtLevel(5))));
            lines.push_back(makeBorder('-'));
            lines.push_back(makeKeyValueRow("Harga Rumah", formatMoney(property->getHouseCost())));
            lines.push_back(makeKeyValueRow("Harga Hotel", formatMoney(property->getHotelCost())));

            if (property->getFestivalDuration() > 0) {
                lines.push_back(makeBorder('-'));
                lines.push_back(
                    makeKeyValueRow(
                        "Festival aktif",
                        "x" + std::to_string(property->getFestivalMultiplier()) +
                            ", sisa " + std::to_string(property->getFestivalDuration()) + " giliran"
                    )
                );
                lines.push_back(
                    makeKeyValueRow(
                        "Sewa saat ini",
                        formatMoney(
                            property->getRentAtLevel(property->getBuildingLevel()) * property->getFestivalMultiplier()
                        )
                    )
                );
            }
        } else if (property->getPropertyType() == PropertyType::RAILROAD) {
            lines.push_back(makeBorder('-'));
            lines.push_back(makeRow("Sewa mengikuti jumlah railroad"));
            lines.push_back(makeRow("yang dimiliki pemilik."));
        } else if (property->getPropertyType() == PropertyType::UTILITY) {
            lines.push_back(makeBorder('-'));
            lines.push_back(makeRow("Sewa = total dadu x multiplier"));
            lines.push_back(makeRow("berdasarkan jumlah utility."));
        }

        lines.push_back(makeBorder('='));
        lines.push_back(makeKeyValueRow("Status", buildStatusLabel(property)));
        lines.push_back(makeBorder('='));
        return lines;
    }

    std::vector<std::string> formatPlayerProperties(const Player& player) {
        std::vector<std::string> lines;
        std::vector<PropertyTile*> ownedProperties = player.getProperties();

        lines.push_back("=== Properti Milik: " + player.getUsername() + " ===");
        if (ownedProperties.empty()) {
            lines.push_back("");
            lines.push_back("Kamu belum memiliki properti apapun.");
            return lines;
        }

        std::sort(ownedProperties.begin(), ownedProperties.end(), propertySorter);

        lines.push_back("");
        std::string currentGroup;
        int totalPropertyWealth = 0;

        for (PropertyTile* property : ownedProperties) {
            if (property == nullptr) {
                continue;
            }

            const std::string group = propertyGroupLabel(property);
            if (group != currentGroup) {
                if (!currentGroup.empty()) {
                    lines.push_back("");
                }
                currentGroup = group;
                lines.push_back("[" + currentGroup + "]");
            }

            lines.push_back(formatPropertyRow(property));
            if (property->getFestivalDuration() > 0) {
                lines.push_back("    * " + festivalSummary(property));
            }
            totalPropertyWealth += propertyWealthValue(property);
        }

        lines.push_back("");
        lines.push_back("Total kekayaan properti: " + formatMoney(totalPropertyWealth));
        return lines;
    }
}
