#pragma once

#include <exception>
#include <string>

class SkillCard;

class NimonspoliException : public std::exception {
protected:
    std::string message;

public:
    explicit NimonspoliException(const std::string& msg) : message(msg) {}
    virtual ~NimonspoliException() = default;

    const char* what() const noexcept override {
        return message.c_str();
    }

    std::string getMessage() const {
        return message;
    }
};

class InsufficientFundsException : public NimonspoliException {
private:
    int required;
    int available;

public:
    InsufficientFundsException(int required, int available)
        : NimonspoliException(
              "Membutuhkan M" + std::to_string(required)
                  + ", tersedia M" + std::to_string(available)),
          required(required),
          available(available) {}

    int getRequired() const {
        return required;
    }

    int getAvailable() const {
        return available;
    }
};

class CardHandFullException : public NimonspoliException {
private:
    std::string playerUsername;
    SkillCard* newCard;

public:
    CardHandFullException(const std::string& username, SkillCard* newCard)
        : NimonspoliException(username + " sudah memiliki 3 kartu"),
          playerUsername(username),
          newCard(newCard) {}

    const std::string& getPlayerUsername() const {
        return playerUsername;
    }

    SkillCard* getNewCard() const {
        return newCard;
    }
};

class InvalidCommandException : public NimonspoliException {
private:
    std::string commandKeyword;
    std::string reason;

public:
    InvalidCommandException(const std::string& keyword, const std::string& reason)
        : NimonspoliException("Perintah '" + keyword + "': " + reason),
          commandKeyword(keyword),
          reason(reason) {}

    const std::string& getCommandKeyword() const {
        return commandKeyword;
    }

    const std::string& getReason() const {
        return reason;
    }
};

class InvalidPropertyException : public NimonspoliException {
private:
    std::string invalidCode;
    std::string reason;

public:
    InvalidPropertyException(const std::string& code, const std::string& reason)
        : NimonspoliException("Properti '" + code + "' tidak valid: " + reason),
          invalidCode(code),
          reason(reason) {}

    const std::string& getInvalidCode() const {
        return invalidCode;
    }

    const std::string& getReason() const {
        return reason;
    }
};

class SkillUseFailedException : public NimonspoliException {
private:
    std::string cardType;

public:
    SkillUseFailedException(const std::string& cardType, const std::string& reason)
        : NimonspoliException(cardType + ": " + reason),
          cardType(cardType) {}

    const std::string& getCardType() const {
        return cardType;
    }
};

class SaveLoadException : public NimonspoliException {
private:
    std::string operation;
    std::string filename;
    std::string detail;

public:
    SaveLoadException(
        const std::string& operation,
        const std::string& filename,
        const std::string& detail
    )
        : NimonspoliException(
              "Gagal " + operation +
              (filename.empty() ? "" : " file \"" + filename + "\"") +
              (detail.empty() ? "." : ": " + detail)),
          operation(operation),
          filename(filename),
          detail(detail) {}

    const std::string& getOperation() const {
        return operation;
    }

    const std::string& getFilename() const {
        return filename;
    }

    const std::string& getDetail() const {
        return detail;
    }
};

class ParseException : public SaveLoadException {
private:
    std::string section;

public:
    ParseException(
        const std::string& filename,
        const std::string& section,
        const std::string& detail
    )
        : SaveLoadException(
              "memuat",
              filename,
              "format tidak valid pada " + section + ": " + detail),
          section(section) {}

    const std::string& getSection() const {
        return section;
    }
};

class ConfigException : public NimonspoliException {
private:
    std::string path;
    std::string detail;

public:
    ConfigException(const std::string& path, const std::string& detail)
        : NimonspoliException(
              path.empty()
                  ? "Konfigurasi permainan tidak valid: " + detail
                  : "Konfigurasi permainan bermasalah pada \"" + path + "\": " + detail),
          path(path),
          detail(detail) {}

    const std::string& getPath() const {
        return path;
    }

    const std::string& getDetail() const {
        return detail;
    }
};

class GameInitException : public NimonspoliException {
public:
    explicit GameInitException(const std::string& detail)
        : NimonspoliException("Inisialisasi game gagal: " + detail) {}
};

class DeckEmptyException : public NimonspoliException {
private:
    std::string deckName;

public:
    explicit DeckEmptyException(const std::string& deckName)
        : NimonspoliException(
              deckName.empty()
                  ? "Deck kartu kosong atau tidak dapat digunakan."
                  : "Deck \"" + deckName + "\" kosong atau tidak dapat digunakan."),
          deckName(deckName) {}

    const std::string& getDeckName() const {
        return deckName;
    }
};
