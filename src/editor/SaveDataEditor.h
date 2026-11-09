#pragma once

#include "EditorContext.h"
#include "GameBuilds.h"

#include <filesystem>
#include <string>
#include <vector>

/*
 * SaveDataEditor
 * --------------
 * The "Save data" window: edits a player.json without playing.
 *   - The project's game_data/player.json is the starting save: every build of the game copies it.
 *   - Each build's game_data/player.json is the progress of the game played from that build.
 * Edits shells, the current chapter, cleared stages and chapters, enemies met, owned fish (level, form),
 * fish waiting to be unlocked and the lineup. Can reset the save to a new game (like the game's first
 * launch: the first fish, nothing else) and copy it to the project and every build.
 * Written in the same format DataLoader::save_player writes.
 */

class SaveDataEditor
{
public:

    bool isOpen = false;

    // Lists the saves (project, then every build) and loads the first one
    void open(const std::filesystem::path& projectFolder, const std::vector<GameBuilds::Build>& builds);

    void draw(EditorContext& ctx);

private:

    struct SaveFile
    {
        std::string label;
        std::filesystem::path path;
    };

    bool load(std::string& error);
    bool write(const std::filesystem::path& path, std::string& error) const;

    // The save of the game's first launch (DataLoader::load_player without a file)
    static ordered_json new_game();

    // Missing parts filled, so the fields can edit them
    void normalize();

    void draw_progress(EditorContext& ctx);
    void draw_units(EditorContext& ctx);
    void draw_enemies(EditorContext& ctx);

    std::vector<SaveFile> m_files;
    int m_current = 0;
    ordered_json m_save;
    bool m_isLoaded = false;
    bool m_hasChanges = false;
};
