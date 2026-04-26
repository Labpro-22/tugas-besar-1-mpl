#include "core/Dice.hpp"

#include <stdexcept>

Dice::Dice()
    : die1(1),
      die2(1),
      rng(std::random_device{}()) {}

void Dice::roll() {
    std::uniform_int_distribution<int> dis(1, 6);
    die1 = dis(rng);
    die2 = dis(rng);
}

void Dice::setManual(int d1, int d2) {
    if (d1 < 1 || d1 > 6 || d2 < 1 || d2 > 6) {
        throw std::out_of_range("Nilai dadu harus berada pada rentang 1 sampai 6.");
    }

    die1 = d1;
    die2 = d2;
}

int Dice::getDie1() const {
    return die1;
}

int Dice::getDie2() const {
    return die2;
}

int Dice::getTotal() const {
    return die1 + die2;
}

bool Dice::isDouble() const {
    return die1 == die2;
}
