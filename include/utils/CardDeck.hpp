#pragma once
 
#include <vector>
#include <string>
#include <algorithm>
#include <random>
#include <stdexcept>

template <typename T>
class CardDeck {
private:
    std::vector<T*> deck;
    std::vector<T*> discard;

public:
    CardDeck() = default;
    ~CardDeck() = default;

    T* draw() {
        if (deck.empty()) {
            if (discard.empty()) {
                throw std::runtime_error("CardDeck: no cards available in deck or discard.");
            }
            reshuffle();
        }

        T* card = deck.back();
        deck.pop_back();
        return card;
    }

    void reshuffle() {
        deck.insert(deck.end(), discard.begin(), discard.end());
        discard.clear();
        std::shuffle(deck.begin(), deck.end(), std::mt19937{std::random_device{}()});
    }

    void discardCard(T* card) {
        discard.push_back(card);
    }

    bool isEmpty() const {
        return deck.empty() && discard.empty();
    }

    int size() const {
        return static_cast<int>(deck.size());
    }

    int getRemaining() const {
        return static_cast<int>(deck.size());
    }

    void initializeDeck(const std::vector<T*>& cards) {
        deck = cards;
        discard.clear();
    }

    std::vector<std::string> getDeckState() const {
        return {};
    }
};
