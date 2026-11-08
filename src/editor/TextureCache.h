#pragma once

#include <SFML/Graphics/Texture.hpp>

#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>

/*
 * TextureCache
 * ------------
 * Loads the project's images on demand ("assets/..." paths, relative to the project folder)
 * and keeps them for the previews. A missing image is remembered so it isn't retried every frame.
 */
class TextureCache
{
public:
    explicit TextureCache(std::filesystem::path projectFolder);

    // The texture, or nullptr if the file is missing or not an image
    const sf::Texture* get(const std::string& relativePath);

    // Reloads this image next time (after it was imported or replaced)
    void forget(const std::string& relativePath);

    void clear();

private:
    std::filesystem::path m_projectFolder;
    std::unordered_map<std::string, std::unique_ptr<sf::Texture>> m_textures;
    std::unordered_set<std::string> m_missing;
};
