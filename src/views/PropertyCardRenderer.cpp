#include "views/PropertyCardRenderer.hpp"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "models/Enums.hpp"
#include "models/tiles/RailroadTile.hpp"
#include "models/tiles/StreetTile.hpp"
#include "models/tiles/UtilityTile.hpp"

namespace {
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

    std::string colorGroupToString(ColorGroup colorGroup) {
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

    std::string getOwnerName(const PropertyTile* property) {
        if (property == nullptr || property->getOwner() == nullptr) {
            return "BANK";
        }

        return property->getOwner()->getUsername();
    }

    void printDivider(char fill = '=') {
        std::cout << std::string(44, fill) << std::endl;
    }

    void printKeyValue(const std::string& key, const std::string& value) {
        std::cout << std::left << std::setw(18) << key << ": " << value << std::endl;
    }

    void printKeyValue(const std::string& key, int value) {
        printKeyValue(key, "M" + std::to_string(value));
    }

    bool propertySorter(const PropertyTile* lhs, const PropertyTile* rhs) {
        if (lhs == nullptr || rhs == nullptr) {
            return lhs < rhs;
        }

        const StreetTile* leftStreet = dynamic_cast<const StreetTile*>(lhs);
        const StreetTile* rightStreet = dynamic_cast<const StreetTile*>(rhs);

        if (leftStreet != nullptr && rightStreet != nullptr &&
            leftStreet->getColorGroup() != rightStreet->getColorGroup()) {
            return static_cast<int>(leftStreet->getColorGroup()) < static_cast<int>(rightStreet->getColorGroup());
        }

        return lhs->getIndex() < rhs->getIndex();
    }
}  // namespace

void PropertyCardRenderer::renderDeed(const PropertyTile* property) const {
    if (property == nullptr) {
        std::cout << "Properti tidak valid." << std::endl;
        return;
    }

    printDivider();
    std::cout << "AKTA KEPEMILIKAN" << std::endl;
    printDivider();
    printKeyValue("Nama", property->getName());
    printKeyValue("Kode", property->getCode());
    printKeyValue("Pemilik", getOwnerName(property));
    printKeyValue("Status", propertyStatusToString(property->getStatus()));
    printKeyValue("Harga Beli", property->getBuyPrice());
    printKeyValue("Nilai Gadai", property->getMortgageValue());

    const StreetTile* street = dynamic_cast<const StreetTile*>(property);
    if (street != nullptr) {
        printKeyValue("Jenis", "STREET");
        printKeyValue("Warna", colorGroupToString(street->getColorGroup()));
        printKeyValue("Level Bangunan", street->getBuildingLevel() == 5
            ? "HOTEL"
            : std::to_string(street->getBuildingLevel()));

        std::cout << "Info Sewa          : mengikuti tabel sewa internal StreetTile." << std::endl;
        std::cout << "Info Bangunan      : level 0-4 = rumah, level 5 = hotel." << std::endl;

        if (street->getFestivalDuration() > 0) {
            std::cout << "Festival Aktif     : x" << street->getFestivalMultiplier()
                      << " (" << street->getFestivalDuration() << " giliran)" << std::endl;
        }

        printDivider('-');
        return;
    }

    const RailroadTile* railroad = dynamic_cast<const RailroadTile*>(property);
    if (railroad != nullptr) {
        printKeyValue("Jenis", "RAILROAD");
        std::cout << "Sewa railroad mengikuti jumlah stasiun milik pemilik." << std::endl;
        printDivider('-');
        return;
    }

    const UtilityTile* utility = dynamic_cast<const UtilityTile*>(property);
    if (utility != nullptr) {
        printKeyValue("Jenis", "UTILITY");
        std::cout << "Sewa utility dihitung dari total dadu x multiplier utilitas." << std::endl;
        printDivider('-');
        return;
    }

    printDivider('-');
}

void PropertyCardRenderer::renderPlayerProperties(const Player& player) const {
    std::vector<PropertyTile*> ownedProperties = player.getProperties();

    std::cout << "=== DAFTAR PROPERTI " << player.getUsername() << " ===" << std::endl;
    if (ownedProperties.empty()) {
        std::cout << "Pemain ini belum memiliki properti." << std::endl;
        return;
    }

    std::sort(ownedProperties.begin(), ownedProperties.end(), propertySorter);

    ColorGroup currentColorGroup = ColorGroup::DEFAULT;
    bool hasStreetGroup = false;
    bool utilityHeaderShown = false;
    bool railroadHeaderShown = false;

    for (std::size_t i = 0; i < ownedProperties.size(); ++i) {
        PropertyTile* property = ownedProperties[i];
        if (property == nullptr) {
            continue;
        }

        StreetTile* street = dynamic_cast<StreetTile*>(property);
        RailroadTile* railroad = dynamic_cast<RailroadTile*>(property);
        UtilityTile* utility = dynamic_cast<UtilityTile*>(property);

        if (street != nullptr) {
            if (!hasStreetGroup || currentColorGroup != street->getColorGroup()) {
                currentColorGroup = street->getColorGroup();
                hasStreetGroup = true;
                std::cout << "[" << colorGroupToString(currentColorGroup) << "]" << std::endl;
            }

            std::cout << "- " << street->getName()
                      << " (" << street->getCode() << ")"
                      << " | Status: " << propertyStatusToString(street->getStatus())
                      << " | Level: ";

            if (street->getBuildingLevel() == 5) {
                std::cout << "HOTEL";
            } else {
                std::cout << street->getBuildingLevel();
            }

            if (street->getFestivalDuration() > 0) {
                std::cout << " | Festival x" << street->getFestivalMultiplier()
                          << " (" << street->getFestivalDuration() << " giliran)";
            }

            std::cout << std::endl;
            continue;
        }

        if (railroad != nullptr) {
            if (!railroadHeaderShown) {
                std::cout << "[RAILROAD]" << std::endl;
                railroadHeaderShown = true;
            }

            std::cout << "- " << railroad->getName()
                      << " (" << railroad->getCode() << ")"
                      << " | Status: " << propertyStatusToString(railroad->getStatus())
                      << std::endl;
            continue;
        }

        if (utility != nullptr) {
            if (!utilityHeaderShown) {
                std::cout << "[UTILITY]" << std::endl;
                utilityHeaderShown = true;
            }

            std::cout << "- " << utility->getName()
                      << " (" << utility->getCode() << ")"
                      << " | Status: " << propertyStatusToString(utility->getStatus())
                      << std::endl;
        }
    }
}

void PropertyCardRenderer::renderAuctionUI(const PropertyTile* property,
                                           int currentBid,
                                           const Player* bidder) const {
    printDivider();
    std::cout << "PANEL LELANG" << std::endl;
    printDivider();

    if (property == nullptr) {
        std::cout << "Tidak ada properti yang sedang dilelang." << std::endl;
        printDivider('-');
        return;
    }

    printKeyValue("Properti", property->getName() + " (" + property->getCode() + ")");
    printKeyValue("Status", propertyStatusToString(property->getStatus()));
    printKeyValue("Harga Saat Ini", currentBid);
    printKeyValue("Penawar Tertinggi", bidder != nullptr ? bidder->getUsername() : "-");
    printKeyValue("Nilai Gadai", property->getMortgageValue());

    const StreetTile* street = dynamic_cast<const StreetTile*>(property);
    if (street != nullptr) {
        printKeyValue("Jenis", "STREET");
        printKeyValue("Warna", colorGroupToString(street->getColorGroup()));
    } else if (dynamic_cast<const RailroadTile*>(property) != nullptr) {
        printKeyValue("Jenis", "RAILROAD");
    } else if (dynamic_cast<const UtilityTile*>(property) != nullptr) {
        printKeyValue("Jenis", "UTILITY");
    }

    printDivider('-');
    std::cout << "Masukkan bid lebih besar dari harga saat ini atau PASS." << std::endl;
}
