#pragma once

#include <SFML/Window/Window.hpp>

#include <filesystem>
#include <optional>

/*
 * FileDialog
 * ----------
 * The system's "Open file" dialog (Windows, like the game), used to import PNG images.
 */
namespace FileDialog
{
    // Lets the user pick a .png file. Empty if cancelled
    std::optional<std::filesystem::path> open_png(const sf::Window& owner, const char* title);
}
