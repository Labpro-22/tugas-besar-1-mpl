#include "models/tiles/CardTile.hpp"

#include "core/GameContext.hpp"
#include "core/GameIO.hpp"
#include "utils/CardDeck.hpp"
#include "models/cards/ActionCard.hpp"
#include "core/TurnManager.hpp"
#include "utils/TransactionLogger.hpp"

CardTile::CardTile() : ActionTile(), cardType(CardType::CHANCE) {}

CardTile::CardTile(int index, const std::string& code, const std::string& name, CardType cardType)
    : ActionTile(index, code, name, TileCategory::DEFAULT), cardType(cardType) {}

void CardTile::onLanded(Player& player, GameContext& gameContext) {
    GameIO* io = gameContext.getIO();
    if (io != nullptr) {
        std::string tileName = cardType == CardType::CHANCE ? "Petak Kesempatan" : "Petak Dana Umum";
        io->showMessage("Kamu mendarat di " + tileName + "!");
        io->showMessage("Mengambil kartu...");
    }

    CardDeck<ActionCard>* deck = (cardType == CardType::CHANCE) 
                                 ? gameContext.getChanceDeck() 
                                 : gameContext.getCommunityDeck();
    if (deck) {
        try {
            ActionCard* card = deck->draw();
            
            if (card) {
                if (io != nullptr) {
                    io->showMessage("Kartu: \"" + card->getText() + "\"");
                }
                card->execute(player, gameContext);
                deck->discardCard(card); 
            }
        } catch (const std::runtime_error& e) {
            if (io != nullptr) {
                io->showError(e, gameContext.getLogger(), gameContext.getTurnManager()->getCurrentTurn(), player.getUsername());
            }
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
