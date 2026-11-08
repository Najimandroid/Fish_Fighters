#pragma once

#include "../external/nlohmann/json.hpp"

#include <array>
#include <filesystem>
#include <string>
#include <vector>

// Keeps the order of the keys when saving, so the JSON files stay readable
using ordered_json = nlohmann::ordered_json;

/*
 * DataFile
 * --------
 * The JSON files of game_data the editor works on (player.json is the player's save: never edited).
 */
enum class DataFile
{
    UNITS,
    ENEMIES,
    STAGES,
    CHAPTERS,
    STATUSES,
    BACKGROUNDS,    // Optional file: an empty one is used if it doesn't exist yet

    COUNT
};

/*
 * ProjectData
 * -----------
 * The game's data as the editor sees it: the JSON documents themselves, so nothing is ever lost
 * (even fields the game doesn't read yet).
 *
 * Responsibilities:
 *  - Loads and saves the JSON files of <project>/game_data (saves only the files that changed,
 *    through a temporary file so a crash can't corrupt them)
 *  - Undo / redo: every finished edit is one step (commit_edit() after a change)
 *  - Helpers to find entries by UID and to resolve asset paths ("assets/...") in the project
 *
 * Editing works like this: widgets change the documents live (previews update while dragging),
 * then call commit_edit() when the edit is finished; that creates the undo step.
 */
class ProjectData
{
public:
    explicit ProjectData(std::filesystem::path projectFolder);

    // Reads every file of game_data. Returns false (see get_error()) if one can't be read
    bool load();

    // Writes the files that changed since the last save
    bool save();

    ordered_json& get(DataFile file);
    const ordered_json& get(DataFile file) const;

    // ===== Undo / redo =====
    void commit_edit(DataFile file);   // Call after a finished edit of `file`
    void commit_all();                 // Commits edits still in progress (a text field being typed in)
    bool undo();
    bool redo();
    bool can_undo() const;
    bool can_redo() const;

    // ===== State =====
    bool has_unsaved_changes() const;
    bool is_unsaved(DataFile file) const;
    const std::string& get_error() const;

    // ===== Paths =====
    const std::filesystem::path& get_project_folder() const;
    std::filesystem::path get_data_folder() const;                       // <project>/game_data
    std::filesystem::path resolve(const std::string& relativePath) const; // <project>/<relativePath>
    bool exists(const std::string& relativePath) const;
    static const char* file_name(DataFile file);                         // "units.json"...

    // ===== Entries by UID ("UID_3": { "UID": 3, ... }) =====
    static std::string uid_key(int uid);
    std::string find_key(DataFile file, int uid) const;                  // Key of that entry ("" if none)
    ordered_json* find_uid(DataFile file, int uid);
    const ordered_json* find_uid(DataFile file, int uid) const;
    std::vector<int> sorted_uids(DataFile file) const;
    int next_free_uid(DataFile file) const;                               // Smallest unused UID >= 1

private:
    static constexpr size_t FILE_COUNT = static_cast<size_t>(DataFile::COUNT);
    static constexpr size_t MAX_UNDO_STEPS = 200;

    struct Edit
    {
        DataFile file;
        ordered_json before;
        ordered_json after;
    };

    std::filesystem::path m_projectFolder;
    std::array<ordered_json, FILE_COUNT> m_documents;   // Live documents (what the widgets edit)
    std::array<ordered_json, FILE_COUNT> m_committed;   // Documents as of the last commit (undo reference)
    std::array<bool, FILE_COUNT> m_isUnsaved{};

    std::vector<Edit> m_undoStack;
    std::vector<Edit> m_redoStack;

    std::string m_error;
};
