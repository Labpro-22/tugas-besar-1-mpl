#include "core/GameStateMapper.hpp"

#include <string>
#include <vector>

#include "core/Board.hpp"
#include "core/TurnManager.hpp"
#include "models/config/ConfigData.hpp"
#include "models/Player.hpp"
#include "models/cards/SkillCard.hpp"
#include "models/state/GameState.hpp"
#include "models/state/PlayerState.hpp"
#include "models/state/PropertyState.hpp"
#include "models/tiles/PropertyTile.hpp"
#include "models/tiles/RailroadTile.hpp"
#include "models/tiles/StreetTile.hpp"
#include "models/tiles/Tile.hpp"
#include "utils/CardDeck.hpp"
#include "utils/TransactionLogger.hpp"
#include "core/DeckFactory.hpp"

GameState GameStateMapper::create(
    const Board& board,
    const std::vector<Player>& players,
    const TurnManager& turnManager,
    const CardDeck<SkillCard>& skillDeck,
    const ConfigData& configData,
    TransactionLogger* logger
) {
    std::vector<PlayerState> playerStates;
    for (const Player& player : players) {
        std::vector<std::string> cards;
        for (SkillCard* card : player.getHand()) {
            if (card != nullptr) {
                cards.push_back(DeckFactory::encodeSkillCard(card));
            }
        }
        playerStates.emplace_back(
            player.getUsername(),
            player.getBalance(),
            player.getPosition(),
            board.getTile(player.getPosition()) == nullptr ? "GO" : board.getTile(player.getPosition())->getCode(),
            player.getStatus(),
            player.getJailTurns(),
            cards
        );
    }

    std::vector<std::string> turnOrder;
    for (Player* player : turnManager.getActivePlayers()) {
        if (player != nullptr) {
            turnOrder.push_back(player->getUsername());
        }
    }

    std::vector<PropertyState> propertyStates;
    for (PropertyTile* property : board.getProperties()) {
        if (property == nullptr) {
            continue;
        }

        int festivalMultiplier = 1;
        int festivalDuration = 0;
        std::string buildingLevel = "0";
        if (property->getPropertyType() == PropertyType::STREET) {
            festivalMultiplier = property->getFestivalMultiplier();
            festivalDuration = property->getFestivalDuration();
            buildingLevel = property->getBuildingLevel() == 5
                ? "H"
                : std::to_string(property->getBuildingLevel());
        }

        propertyStates.emplace_back(
            property->getCode(),
            property->getPropertyType(),
            property->getOwner() == nullptr ? "BANK" : property->getOwner()->getUsername(),
            property->getStatus(),
            festivalMultiplier,
            festivalDuration,
            buildingLevel
        );
    }

    std::vector<LogEntry> logEntries;
    if (logger != nullptr) {
        const std::vector<LogEntry>& all = logger->getAll();
        logEntries.assign(all.begin(), all.end());
    }

    Player* activePlayer = turnManager.getCurrentPlayer();
    return GameState(
        turnManager.getCurrentTurn(),
        turnManager.getMaxTurn(),
        playerStates,
        turnOrder,
        activePlayer == nullptr ? "" : activePlayer->getUsername(),
        propertyStates,
        skillDeck.getDeckState(),
        logEntries,
        configData.getSourcePath()
    );
}
