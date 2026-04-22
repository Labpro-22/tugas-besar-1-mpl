#include "models/tiles/CardTile.hpp"
#include "core/GameContext.hpp"
#include "utils/CardDeck.hpp"
#include "models/cards/ActionCard.hpp"
#include "core/TurnManager.hpp"
#include "utils/TransactionLogger.hpp"

CardTile::CardTile() : ActionTile(), cardType(CardType::CHANCE) {}

CardTile::CardTile(int index, const std::string& code, const std::string& name, CardType cardType)
    : ActionTile(index, code, name, TileCategory::DEFAULT), cardType(cardType) {}

void CardTile::onLanded(Player& player, GameContext& gameContext) {
    CardDeck<ActionCard>* deck = (cardType == CardType::CHANCE) 
                                 ? gameContext.getChanceDeck() 
                                 : gameContext.getCommunityDeck();
    if (deck) {
        try {
            // Ambil top card
            ActionCard* card = deck->draw();
            
            if (card) {
                card->execute(player, gameContext);
                deck->discardCard(card); 
            }
        } catch (const std::runtime_error& e) {
            // pake logger via getTurnManager
            if (gameContext.getLogger()) {
                gameContext.getLogger()->log(
                    gameContext.getTurnManager()->getCurrentTurn(),
                    player.getUsername(),
                    "ERROR",
                    "Tumpukan kartu kosong!"
                );
            }
        }
    }
}

std::string CardTile::getDisplayLabel() const {
    return getCode(); // "KSP" atau "DNU"
}