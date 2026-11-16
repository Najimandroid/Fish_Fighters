#pragma once

#include <SFML/Window/Window.hpp>

#include <filesystem>
#include <optional>

/*
 * FileDialog
 * ----------
 * The system's "Open file" dialog (Windows, like the game), used to import PNG images and FireAlpaca drawings.
 */
namespace FileDialog
{
    enum class Kind { PNG, MDP };

    // Lets the user pick a .png file. Empty if cancelled
    std::optional<std::filesystem::path> open_png(const sf::Window& owner, const char* title);
    // Lets the user pick a file of that kind (.png, or .mdp: a FireAlpaca / MediBang drawing). Empty if cancelled
    std::optional<std::filesystem::path> open(const sf::Window& owner, const char* title, Kind kind);
}
