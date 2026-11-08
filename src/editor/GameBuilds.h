#pragma once

#include <filesystem>
#include <string>
#include <vector>

/*
 * GameBuilds
 * ----------
 * The game's builds of this project (out/build/<config>/Fish_Fighters.exe).
 *
 * The build copies game_data and assets next to the executable, so without this the game
 * would only see the editor's changes after a rebuild. The editor copies them on save
 * (never player.json: that's the player's progress) and can launch the game.
 */
namespace GameBuilds
{
    struct Build
    {
        std::string name;                // "x64-Debug"
        std::filesystem::path folder;
    };

    std::vector<Build> find(const std::filesystem::path& projectFolder);

    // Copies the game_data JSON files (except player.json) and the assets folder into the build
    bool copy_data(const std::filesystem::path& projectFolder, const Build& build, std::string& error);

    // Starts the game from its folder (it reads its files relative to it)
    bool launch(const Build& build, std::string& error);
}
