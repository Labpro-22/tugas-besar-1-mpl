#include "models/config/TaxConfig.hpp"

TaxConfig::TaxConfig() : pphFlat(0), pphPercentage(0), pbmFlat(0) {}

TaxConfig::TaxConfig(int pphFlat, int pphPercentage, int pbmFlat)
    : pphFlat(pphFlat), pphPercentage(pphPercentage), pbmFlat(pbmFlat) {}

int TaxConfig::getPphFlat() const {
    return pphFlat;
}

int TaxConfig::getPphPercentage() const {
    return pphPercentage;
}

int TaxConfig::getPbmFlat() const {
    return pbmFlat;
}
