#include "models/tiles/PropertyTile.hpp"

#include <algorithm>
#include <vector>

#include "models/Player.hpp"

PropertyTile::PropertyTile() : Tile(), owner(nullptr), status(PropertyStatus::BANK), mortgageValue(0) {}

PropertyTile::PropertyTile(int index,
                           const std::string& code,
                           const std::string& name,
                           int buyPrice,
                           int mortgageValue)
    : Tile(index, code, name, TileCategory::PROPERTY),
      owner(nullptr),
      status(PropertyStatus::BANK),
      mortgageValue(mortgageValue),
      buyPrice(buyPrice) {}

void PropertyTile::mortgage() {
    // Properti diubah statusnya menjadi tergadai. Penambahan uang pemain 
    // idealnya ditangani oleh pemanggil fungsi (GameContext/Player).
    if (status == PropertyStatus::OWNED) {
        status = PropertyStatus::MORTGAGED;
    }
}

void PropertyTile::redeem() {
    if (status == PropertyStatus::MORTGAGED) {
        status = PropertyStatus::OWNED;
    }
}

void PropertyTile::transferTo(Player& newOwner) {
    if (owner != nullptr && owner != &newOwner) {
        owner->removeProperty(this);
    }

    owner = &newOwner;
    status = PropertyStatus::OWNED;

    const std::vector<PropertyTile*>& ownedProperties = newOwner.getProperties();
    if (std::find(ownedProperties.begin(), ownedProperties.end(), this) == ownedProperties.end()) {
        newOwner.addProperty(this);
    }
}

void PropertyTile::returnToBank() {
    if (owner != nullptr) {
        owner->removeProperty(this);
    }
    owner = nullptr;
    status = PropertyStatus::BANK;
}

bool PropertyTile::isOwnedBy(const Player& player) const {
    return owner == &player;
}

bool PropertyTile::isMortgaged() const {
    return status == PropertyStatus::MORTGAGED;
}

Player* PropertyTile::getOwner() const {
    return owner;
}

PropertyStatus PropertyTile::getStatus() const {
    return status;
}

int PropertyTile::getMortgageValue() const {
    return mortgageValue;
}

int PropertyTile::getBuyPrice() const{
    return buyPrice;
}

ColorGroup PropertyTile::getColorGroup() const {
    return ColorGroup::DEFAULT;
}

int PropertyTile::getBuildingLevel() const {
    return 0;
}

int PropertyTile::getFestivalMultiplier() const {
    return 1;
}

int PropertyTile::getFestivalDuration() const {
    return 0;
}

int PropertyTile::getHouseCost() const {
    return 0;
}

int PropertyTile::getHotelCost() const {
    return 0;
}

int PropertyTile::getRentAtLevel(int) const {
    return 0;
}
