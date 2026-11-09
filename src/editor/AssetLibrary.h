#pragma once

#include <SFML/Graphics/Image.hpp>
#include <SFML/Graphics/Texture.hpp>

#include <array>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

class ProjectData;

/*
 * ImageFolder
 * -----------
 * Where each kind of image lives in the project (the game reads them from there).
 */
enum class ImageFolder
{
    UNIT_SHEETS,    // assets/images/textures/units
    ENEMY_SHEETS,   // assets/images/textures/enemies
    BASES,              // assets/images/textures/bases
    STAGE_BACKGROUNDS,  // assets/images/textures/backgrounds/stages (battlefields)
    MAPS,               // assets/images/textures/backgrounds/maps (chapter maps)
    MENU_BACKGROUNDS,   // assets/images/textures/backgrounds/menus (chosen in UIManager's code)
    UNIT_ICONS,         // assets/images/textures/icons/units (icon_<UID>_<form>.png)

    COUNT
};

/*
 * AssetLibrary
 * ------------
 * The project's images: lists them per folder and brings new ones in.
 *
 * Nothing is ever overwritten silently: imported files get a free name (_2, _3...),
 * and a replaced file (a unit icon) is kept next to it as "name(old).png".
 */
class AssetLibrary
{
public:
    static constexpr unsigned UNIT_ICON_WIDTH = 200;   // Size of the deploy icons the game draws
    static constexpr unsigned UNIT_ICON_HEIGHT = 150;

    explicit AssetLibrary(const ProjectData& project);

    static const char* folder(ImageFolder folder);       // Relative to the project
    static const char* display_name(ImageFolder folder); // "Unit sprite sheets"...
    static std::optional<ImageFolder> folder_of(const std::string& path); // Folder an "assets/..." image is in
    static std::string unit_icon_path(int uid, int form); // Same path as DataLoader::get_unit_icon_texture_path

    // Every .png of the folder ("assets/..." paths), sorted, without the "(old)" backups. Cached until refresh()
    const std::vector<std::string>& list(ImageFolder folder);
    void refresh();

    // Copies `source` into the folder and returns its "assets/..." path.
    // A file already in that folder is used as it is
    std::optional<std::string> import(ImageFolder folder, const std::filesystem::path& source, std::string& error);

    // Writes `source` at `relativePath`, keeping the previous file as "name(old).png"
    bool replace(const std::string& relativePath, const std::filesystem::path& source, std::string& error);
    bool replace(const std::string& relativePath, const sf::Image& image, std::string& error);

    // A deploy icon made from the first frame of a sprite sheet, centered on a transparent background
    static std::optional<sf::Image> make_icon(const sf::Texture& sheet, int frameCount);

    // Files written since the last call ("assets/..." paths): the EditorApp copies them into the game builds
    std::vector<std::string> take_written_files();

private:
    bool back_up(const std::filesystem::path& path, std::string& error);

    static constexpr size_t FOLDER_COUNT = static_cast<size_t>(ImageFolder::COUNT);

    const ProjectData& m_project;
    std::array<std::vector<std::string>, FOLDER_COUNT> m_lists;
    std::array<bool, FOLDER_COUNT> m_isListed{};
    std::vector<std::string> m_writtenFiles;
};
