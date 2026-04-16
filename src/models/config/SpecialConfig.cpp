#include "models/config/SpecialConfig.hpp"

SpecialConfig::SpecialConfig() : goSalary(0), jailFine(0) {}

SpecialConfig::SpecialConfig(int goSalary, int jailFine)
    : goSalary(goSalary), jailFine(jailFine) {}

int SpecialConfig::getGoSalary() const {
    return goSalary;
}

int SpecialConfig::getJailFine() const {
    return jailFine;
}
