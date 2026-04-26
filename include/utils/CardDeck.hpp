#pragma once
 
#include <vector>
#include <string>
#include <algorithm>
#include <random>
#include <utility>

#include "utils/exceptions/NimonspoliException.hpp"

template <typename T>
class CardDeck {
private:
    std::vector<T*> deck;
    std::vector<T*> discard;
    std::vector<T*> ownedCards;

    void clearOwnedCards() {
        for (T* card : ownedCards) {
            delete card;
        }
        ownedCards.clear();
        deck.clear();
        discard.clear();
    }

public:
    CardDeck() = default;
    ~CardDeck() {
        clearOwnedCards();
    }

    CardDeck(const CardDeck&) = delete;
    CardDeck& operator=(const CardDeck&) = delete;

    CardDeck(CardDeck&& other) noexcept
        : deck(std::move(other.deck)),
          discard(std::move(other.discard)),
          ownedCards(std::move(other.ownedCards)) {
        other.deck.clear();
        other.discard.clear();
        other.ownedCards.clear();
    }

    CardDeck& operator=(CardDeck&& other) noexcept {
        if (this != &other) {
            clearOwnedCards();
            deck = std::move(other.deck);
            discard = std::move(other.discard);
            ownedCards = std::move(other.ownedCards);
            other.deck.clear();
            other.discard.clear();
            other.ownedCards.clear();
        }
        return *this;
    }

    T* draw() {
        if (deck.empty()) {
            if (discard.empty()) {
                throw DeckEmptyException("kartu");
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
        if (card != nullptr) {
            discard.push_back(card);
        }
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
        clearOwnedCards();
        deck = cards;
        ownedCards = cards;
        discard.clear();
    }

    void adoptCard(T* card) {
        if (card != nullptr) {
            ownedCards.push_back(card);
        }
    }

    std::vector<std::string> getDeckState() const {
        std::vector<std::string> state;
        for (T* card : deck) {
            if (card != nullptr) {
                state.push_back(card->getTypeName());
            }
        }
        return state;
    }
};
