#include "views/PropertyCardRenderer.hpp"

#include <iostream>
#include <string>
#include <vector>

#include "views/CliOutputFormatter.hpp"

namespace {
    void printLines(const std::vector<std::string>& lines) {
        for (const std::string& line : lines) {
            std::cout << line << std::endl;
        }
    }
}

void PropertyCardRenderer::renderDeed(const PropertyTile* property) const {
    printLines(CliOutputFormatter::formatPropertyDeed(property));
}

void PropertyCardRenderer::renderPlayerProperties(const Player& player) const {
    printLines(CliOutputFormatter::formatPlayerProperties(player));
}
