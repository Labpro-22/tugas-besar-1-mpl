#pragma once

enum class TileCategory {
    PROPERTY,
    ACTION,
    SPECIAL
};

enum class PropertyType {
    STREET,
    RAILROAD,
    UTILITY
};

enum class ColorGroup {
    COKLAT,
    BIRU_MUDA,
    MERAH_MUDA,
    ORANGE,
    MERAH,
    KUNING,
    HIJAU,
    BIRU_TUA,
    ABU_ABU,
    DEFAULT
};

enum class PlayerStatus {
    ACTIVE,
    JAILED,
    BANKRUPT
};

enum class PropertyStatus {
    BANK,
    OWNED,
    MORTGAGED
};

enum class TaxType {
    PPH,
    PBM
};

enum class CardType {
    CHANCE,
    COMMUNITY_CHEST
};