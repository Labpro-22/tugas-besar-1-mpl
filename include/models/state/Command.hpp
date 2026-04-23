#pragma once

#include <string>
#include <vector>

class Command {
private:
    std::string keyword;
    std::vector<std::string> args;

public:
    Command();
    Command(const std::string& keyword, const std::vector<std::string>& args);

    const std::string& getKeyword() const;
    std::vector<std::string>& getArgs();
    const std::vector<std::string>& getArgs() const;
    std::string getArg(int index) const;
    int getArgCount() const;
    bool isKnown() const;
    bool isValid() const;
    std::string getUsage() const;
};
