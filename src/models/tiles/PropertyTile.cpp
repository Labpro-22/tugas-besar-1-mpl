#include "models/tiles/PropertyTile.hpp"

PropertyTile::PropertyTile() : Tile(), owner(nullptr), status(PropertyStatus::BANK), mortgageValue(0) {}

PropertyTile::PropertyTile(int index, const std::string& code, const std::string& name, int mortgageValue, int buyPrice)
    : Tile(index, code, name, TileCategory::PROPERTY), owner(nullptr), status(PropertyStatus::BANK), mortgageValue(mortgageValue), buyPrice(buyPrice) {}

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
    owner = &newOwner;
    status = PropertyStatus::OWNED;
}

void PropertyTile::returnToBank() {
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