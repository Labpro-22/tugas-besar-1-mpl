#pragma once

#include <vector>
#include <string>

class Player;

class TurnManager {
private:
    std::vector<Player*> turnOrder;
    int currentIndex;
    int currentTurn;
    int maxTurn;

public:
    TurnManager(int maxTurn);
    void initializeTurnOrder(const std::vector<Player*>& players);
    void restoreTurnOrder(const std::vector<std::string>& orderedUsernames, const std::vector<Player*>& allPlayers);
    Player* getCurrentPlayer() const;
    void advance();
    bool isMaxTurnReached() const;
    void removePlayer(Player* player);
    std::vector<Player*> getActivePlayers() const;
    Player* getPlayerAfter(Player* player) const;

    int getCurrentTurn() const;
    int getMaxTurn() const;
    void setCurrentTurn(int turn);
    void setCurrentIndex(int index);

    void handlePropertyLanded(Player& player, PropertyTile& tile, GameContext& context);
    void handleRentPayment(Player& player, PropertyTile& tile, GameContext& context);
};
