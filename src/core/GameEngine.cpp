#include "core/GameEngine.hpp"

GameEngine::GameEngine(TransactionLogger* logger)
    : turnManager(0),
      logger(logger),
      context(nullptr),
      configData(nullptr) {

} 

GameEngine::~GameEngine() {
    delete context;
}

void GameEngine::initialize(const ConfigData& configData, const std::vector<std::string>& playerNames) {
    this->configData = &configData;
    (void)playerNames;
}

void GameEngine::startNewGame() {

}

void GameEngine::loadGame(const GameState& gameState) {
    (void)gameState;
}

void GameEngine::runGameLoop() {

}

void GameEngine::processTurn(Player& player) {
    (void)player;
}

void GameEngine::processCommand(const Command& cmd, Player& player) {
    (void)cmd;
    (void)player;
}

bool GameEngine::checkGameEnd() const {
    return false;
}

std::vector<Player*> GameEngine::determineWinner() const {
    return {};
}

void GameEngine::distributeSkillCards() {
}

void GameEngine::buildBoard() {
}

void GameEngine::buildDecks() {
}

void GameEngine::randomizeTurnOrder() {
}
