#include "models/tiles/StreetTile.hpp"

StreetTile::StreetTile() : PropertyTile(), colorGroup(ColorGroup::DEFAULT), houseCost(0), hotelCost(0), buildingLevel(0), festivalMultiplier(1), festivalDuration(0) {
    for(int i=0; i<6; i++) rentTable[i] = 0;
}

StreetTile::StreetTile(
    int index, const std::string& code, const std::string& name, 
    ColorGroup colorGroup, int buyPrice, int mortgageValue, 
    int houseCost, int hotelCost, const int rentTbl[6]
) : PropertyTile(index, code, name, buyPrice, mortgageValue), 
    colorGroup(colorGroup), houseCost(houseCost), 
    hotelCost(hotelCost), buildingLevel(0), festivalMultiplier(1), festivalDuration(0) 
{
    for(int i = 0; i < 6; i++) {
        rentTable[i] = rentTbl[i];
    }
}

void StreetTile::onLanded(Player& player, GameContext& gameContext) {
    // Logika pendaratan dasar (Trigger Beli/Sewa/Lelang) 
    // biasanya diorkestrasi melalui GameContext, namun di sini
    // mendelegasikan event jika diperlukan.
    // gameContext.triggerStreetEvent(player, *this);
}

int StreetTile::calculateRent(int diceTotal, const GameContext& gameContext) {
    if (getStatus() != PropertyStatus::OWNED) return 0;

    int rent = rentTable[buildingLevel];

    // Jika monopoli dan belum ada bangunan, sewa dikali 2
    // if (buildingLevel == 0 && gameContext.hasMonopoly(*getOwner(), colorGroup)) {
    //     rent *= 2;
    // }

    // // Terapkan multiplier festival
    // return rent * festivalMultiplier;
}

void StreetTile::build() {
    if (buildingLevel < 5) {
        buildingLevel++;
    }
}

int StreetTile::sellBuilding() {
    if (buildingLevel > 0) {
        buildingLevel--;
        return houseCost / 2;
    }
    return 0;
}

void StreetTile::applyFestival() {
    // Maksimal kenaikan adalah 8x (2 -> 4 -> 8)
    if (festivalMultiplier < 8) {
        festivalMultiplier *= 2;
    } 
    else{}
    // Durasi di-reset menjadi 3 giliran
    festivalDuration = 3;
}

void StreetTile::tickFestival() {
    if (festivalDuration > 0) {
        festivalDuration--;
        if (festivalDuration == 0) {
            festivalMultiplier = 1;
        }
    }
}

int StreetTile::getBuildingLevel() const { return buildingLevel; }
ColorGroup StreetTile::getColorGroup() const { return colorGroup; }
int StreetTile::getFestivalMultiplier() const { return festivalMultiplier; }
int StreetTile::getFestivalDuration() const { return festivalDuration; }
bool StreetTile::canBuildNext() const { return buildingLevel < 5; }

int StreetTile::getSellValueToBank() const {
    // Harga jual = Harga dasar + (1/2 harga total bangunan yang terpasang)
    // int totalBuildingValue = (buildingLevel * houseCost) / 2; // Asumsi hotel = houseCost secara nilai
    // return getBuyPrice() + totalBuildingValue;
}

std::string StreetTile::getDisplayLabel() const {
    return "[" + getCode() + "] " + getName();
}

int StreetTile::getHouseCost() const {
    return houseCost;
}

int StreetTile::getHotelCost() const {
    return hotelCost;
}