#include "models/tiles/CardTile.hpp"

#include "core/GameContext.hpp"
#include "core/TurnManager.hpp"
#include "models/cards/ActionCard.hpp"
#include "utils/CardDeck.hpp"
#include "utils/exceptions/NimonspoliException.hpp"
#include "utils/TransactionLogger.hpp"

CardTile::CardTile() : ActionTile(), cardType(CardType::CHANCE) {}

CardTile::CardTile(int index, const std::string& code, const std::string& name, CardType cardType)
    : ActionTile(index, code, name), cardType(cardType) {}

void CardTile::onLanded(Player& player, GameContext& gameContext) {
    std::string tileName = cardType == CardType::CHANCE ? "Petak Kesempatan" : "Petak Dana Umum";
    gameContext.showMessage("Kamu mendarat di " + tileName + "!");
    gameContext.showMessage("Mengambil kartu...");

    CardDeck<ActionCard>* deck = (cardType == CardType::CHANCE) 
                                 ? gameContext.getChanceDeck() 
                                 : gameContext.getCommunityDeck();
    if (deck) {
        try {
            ActionCard* card = deck->draw();
            
            if (card) {
                if (gameContext.getLogger() != nullptr) {
                    int currentTurn = 0;
                    if (gameContext.getTurnManager() != nullptr) {
                        currentTurn = gameContext.getTurnManager()->getCurrentTurn();
                    }
                    gameContext.getLogger()->log(
                        currentTurn,
                        player.getUsername(),
                        "KARTU",
                        "Mengambil kartu " + tileName + ": " + card->getText()
                    );
                }
                gameContext.showActionCard(cardType, *card);
                card->execute(player, gameContext);
                deck->discardCard(card); 
            }
        } catch (const DeckEmptyException& e) {
            gameContext.showError(e, player.getUsername());
            if (gameContext.getLogger() != nullptr) {
                int currentTurn = 0;
                if (gameContext.getTurnManager() != nullptr) {
                    currentTurn = gameContext.getTurnManager()->getCurrentTurn();
                }
                gameContext.getLogger()->log(
                    currentTurn,
                    player.getUsername(),
                    "ERROR",
                    e.getMessage()
                );
            }
        }
    }
}
