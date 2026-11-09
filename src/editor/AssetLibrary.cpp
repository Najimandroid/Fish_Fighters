#include "AssetLibrary.h"

#include "ProjectData.h"

#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/Sprite.hpp>

#include <algorithm>
#include <cctype>

namespace fs = std::filesystem;

namespace
{
    bool is_png(const fs::path& path)
    {
        std::string extension = path.extension().string();
        std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return extension == ".png";
    }

    // "name.png", then "name_2.png", "name_3.png"... until the name is free
    fs::path free_path(const fs::path& folder, const fs::path& fileName)
    {
        fs::path candidate = folder / fileName;
        for (int i = 2; fs::exists(candidate); ++i)
            candidate = folder / (fileName.stem().string() + "_" + std::to_string(i) + fileName.extension().string());
        return candidate;
    }
}

AssetLibrary::AssetLibrary(const ProjectData& project) :
    m_project(project)
{
}

// ==================
// Folders
// ==================
const char* AssetLibrary::folder(ImageFolder folder)
{
    switch (folder)
    {
    case ImageFolder::UNIT_SHEETS:  return "assets/images/textures/units";
    case ImageFolder::ENEMY_SHEETS: return "assets/images/textures/enemies";
    case ImageFolder::BASES:        return "assets/images/textures/bases";
    case ImageFolder::STAGE_BACKGROUNDS: return "assets/images/textures/backgrounds/stages";
    case ImageFolder::MAPS:         return "assets/images/textures/backgrounds/maps";
    case ImageFolder::MENU_BACKGROUNDS: return "assets/images/textures/backgrounds/menus";
    case ImageFolder::UNIT_ICONS:   return "assets/images/textures/icons/units";
    default:                        return "assets";
    }
}

std::optional<ImageFolder> AssetLibrary::folder_of(const std::string& path)
{
    const std::string directory = path.substr(0, path.find_last_of('/'));
    for (int i = 0; i < static_cast<int>(ImageFolder::COUNT); ++i)
    {
        if (directory == folder(static_cast<ImageFolder>(i))) return static_cast<ImageFolder>(i);
    }
    return std::nullopt;
}

const char* AssetLibrary::display_name(ImageFolder folder)
{
    switch (folder)
    {
    case ImageFolder::UNIT_SHEETS:  return "Unit sprite sheets";
    case ImageFolder::ENEMY_SHEETS: return "Enemy sprite sheets";
    case ImageFolder::BASES:        return "Bases";
    case ImageFolder::STAGE_BACKGROUNDS: return "Stage backgrounds";
    case ImageFolder::MAPS:         return "Chapter maps";
    case ImageFolder::MENU_BACKGROUNDS: return "Menu backgrounds";
    case ImageFolder::UNIT_ICONS:   return "Unit icons";
    default:                        return "Images";
    }
}

std::string AssetLibrary::unit_icon_path(int uid, int form)
{
    return std::string(folder(ImageFolder::UNIT_ICONS)) + "/icon_" + std::to_string(uid) + "_" + std::to_string(form) + ".png";
}

// ==================
// Listing
// ==================
const std::vector<std::string>& AssetLibrary::list(ImageFolder imageFolder)
{
    const size_t i = static_cast<size_t>(imageFolder);
    if (m_isListed[i]) return m_lists[i];

    m_lists[i].clear();
    std::error_code error;
    const fs::path directory = m_project.resolve(folder(imageFolder));
    for (const auto& entry : fs::directory_iterator(directory, error))
    {
        if (!entry.is_regular_file() || !is_png(entry.path())) continue;
        if (entry.path().stem().string().find("(old") != std::string::npos) continue; // Backups of replaced files
        m_lists[i].push_back(std::string(folder(imageFolder)) + "/" + entry.path().filename().generic_string());
    }
    std::sort(m_lists[i].begin(), m_lists[i].end());

    m_isListed[i] = true;
    return m_lists[i];
}

void AssetLibrary::refresh()
{
    m_isListed.fill(false);
}

// ==================
// Import / Replace
// ==================
std::optional<std::string> AssetLibrary::import(ImageFolder imageFolder, const fs::path& source, std::string& error)
{
    if (!is_png(source))
    {
        error = "Only .png images can be imported";
        return std::nullopt;
    }

    const fs::path directory = m_project.resolve(folder(imageFolder));
    std::error_code fsError;
    fs::create_directories(directory, fsError);

    // Already in the right folder: nothing to copy
    if (fs::equivalent(source.parent_path(), directory, fsError))
        return std::string(folder(imageFolder)) + "/" + source.filename().generic_string();

    const fs::path destination = free_path(directory, source.filename());
    fs::copy_file(source, destination, fsError);
    if (fsError)
    {
        error = "Can't copy " + source.string() + ": " + fsError.message();
        return std::nullopt;
    }

    refresh();
    const std::string imported = std::string(folder(imageFolder)) + "/" + destination.filename().generic_string();
    m_writtenFiles.push_back(imported);
    return imported;
}

bool AssetLibrary::back_up(const fs::path& path, std::string& error)
{
    if (!fs::exists(path)) return true;

    // "icon_1_0.png" becomes "icon_1_0(old).png", then "(old 2)"...
    fs::path backup = path.parent_path() / (path.stem().string() + "(old)" + path.extension().string());
    for (int i = 2; fs::exists(backup); ++i)
        backup = path.parent_path() / (path.stem().string() + "(old " + std::to_string(i) + ")" + path.extension().string());

    std::error_code fsError;
    fs::rename(path, backup, fsError);
    if (fsError)
    {
        error = "Can't keep the previous " + path.filename().string() + ": " + fsError.message();
        return false;
    }
    return true;
}

bool AssetLibrary::replace(const std::string& relativePath, const fs::path& source, std::string& error)
{
    if (!is_png(source))
    {
        error = "Only .png images can be imported";
        return false;
    }

    const fs::path destination = m_project.resolve(relativePath);
    std::error_code fsError;
    fs::create_directories(destination.parent_path(), fsError);
    if (fs::equivalent(source, destination, fsError)) return true;

    if (!back_up(destination, error)) return false;

    fs::copy_file(source, destination, fsError);
    if (fsError)
    {
        error = "Can't copy " + source.string() + ": " + fsError.message();
        return false;
    }

    refresh();
    m_writtenFiles.push_back(relativePath);
    return true;
}

bool AssetLibrary::replace(const std::string& relativePath, const sf::Image& image, std::string& error)
{
    const fs::path destination = m_project.resolve(relativePath);
    std::error_code fsError;
    fs::create_directories(destination.parent_path(), fsError);

    if (!back_up(destination, error)) return false;

    if (!image.saveToFile(destination))
    {
        error = "Can't write " + destination.string();
        return false;
    }

    refresh();
    m_writtenFiles.push_back(relativePath);
    return true;
}

std::vector<std::string> AssetLibrary::take_written_files()
{
    std::vector<std::string> files;
    files.swap(m_writtenFiles);
    return files;
}

std::optional<sf::Image> AssetLibrary::make_icon(const sf::Texture& sheet, int frameCount)
{
    const int frameWidth = static_cast<int>(sheet.getSize().x) / std::max(1, frameCount);
    const int frameHeight = static_cast<int>(sheet.getSize().y);
    if (frameWidth <= 0 || frameHeight <= 0) return std::nullopt;

    sf::RenderTexture canvas;
    if (!canvas.resize({ UNIT_ICON_WIDTH, UNIT_ICON_HEIGHT })) return std::nullopt;
    canvas.clear(sf::Color::Transparent);

    // First frame (idle), scaled to fit with a small margin
    sf::Sprite sprite(sheet, sf::IntRect({ 0, 0 }, { frameWidth, frameHeight }));
    const float margin = 10.f;
    const float scale = std::min((UNIT_ICON_WIDTH - 2 * margin) / frameWidth, (UNIT_ICON_HEIGHT - 2 * margin) / frameHeight);
    sprite.setScale({ scale, scale });
    sprite.setPosition({ (UNIT_ICON_WIDTH - frameWidth * scale) / 2.f, (UNIT_ICON_HEIGHT - frameHeight * scale) / 2.f });

    canvas.draw(sprite);
    canvas.display();
    return canvas.getTexture().copyToImage();
}
