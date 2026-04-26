#pragma once

class Board;
class ConfigData;

class BoardFactory {
public:
    static void build(Board& board, const ConfigData& configData);
};
