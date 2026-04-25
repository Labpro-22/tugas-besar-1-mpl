#include "models/tiles/StreetTile.hpp"

#include <stdexcept>

#include "core/GameContext.hpp"
#include "models/Player.hpp"

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
    gameContext.triggerStreetEvent(player, *this);
}

int StreetTile::calculateRent(int, const GameContext& gameContext) {
    if (getStatus() != PropertyStatus::OWNED){
        return 0;
    } 
    int rent = rentTable[buildingLevel];
    if (buildingLevel == 0 && gameContext.hasMonopoly(*getOwner(), colorGroup)) {
        rent *= 2;
    }
    return rent * festivalMultiplier;
}

StreetTile* StreetTile::asStreetTile() {
    return this;
}

const StreetTile* StreetTile::asStreetTile() const {
    return this;
}

void StreetTile::build() {
    if (buildingLevel < 5) {
        buildingLevel++;
    }
}

int StreetTile::sellBuilding() {
    if (buildingLevel > 0) {
        int received = buildingLevel == 5 ? hotelCost / 2 : houseCost / 2;
        buildingLevel--;
        return received;
    }
    return 0;
}

void StreetTile::applyFestival() {
    if (festivalMultiplier < 8) {
        festivalMultiplier *= 2;
    } 
    else{}
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
    int totalBuildingValue = 0;
    if (buildingLevel > 0 && buildingLevel <= 4) {
        totalBuildingValue = (buildingLevel * houseCost) / 2;
    } else if (buildingLevel == 5) {
        totalBuildingValue = ((4 * houseCost) + hotelCost) / 2;
    }
    return getBuyPrice() + totalBuildingValue;
}

PropertyType StreetTile::getPropertyType() const {
    return PropertyType::STREET;
}

int StreetTile::getDevelopmentValue() const {
    if (buildingLevel > 0 && buildingLevel <= 4) {
        return buildingLevel * houseCost;
    }
    if (buildingLevel == 5) {
        return (4 * houseCost) + hotelCost;
    }
    return 0;
}

int StreetTile::getHouseCost() const {
    return houseCost;
}

int StreetTile::getHotelCost() const {
    return hotelCost;
}

int StreetTile::getRentAtLevel(int level) const {
    if (level < 0 || level > 5) {
        throw std::out_of_range("level sewa harus berada pada rentang 0..5");
    }
    return rentTable[level];
}

void StreetTile::setBuildingLevel(int level) {
    if (level < 0) {
        buildingLevel = 0;
    } else if (level > 5) {
        buildingLevel = 5;
    } else {
        buildingLevel = level;
    }
}

void StreetTile::setFestivalState(int multiplier, int duration) {
    festivalMultiplier = (multiplier < 1) ? 1 : multiplier;
    festivalDuration = (duration < 0) ? 0 : duration;
    if (festivalDuration == 0) {
        festivalMultiplier = 1;
    }
}
