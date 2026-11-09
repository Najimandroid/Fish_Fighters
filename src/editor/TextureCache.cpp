#include "TextureCache.h"

#include "../utils/ImageTools.h"

#include <SFML/Graphics/Image.hpp>

TextureCache::TextureCache(std::filesystem::path projectFolder) :
    m_projectFolder(std::move(projectFolder))
{
}

const sf::Texture* TextureCache::get(const std::string& relativePath)
{
    if (relativePath.empty() || m_missing.count(relativePath)) return nullptr;

    auto it = m_textures.find(relativePath);
    if (it != m_textures.end()) return it->second.get();

    auto texture = std::make_unique<sf::Texture>();
    if (!texture->loadFromFile(m_projectFolder / relativePath))
    {
        m_missing.insert(relativePath);
        return nullptr;
    }

    const sf::Texture* result = texture.get();
    m_textures.emplace(relativePath, std::move(texture));
    return result;
}

const sf::Texture* TextureCache::get_seamless(const std::string& relativePath, int overlap)
{
    if (overlap <= 0) return get(relativePath);
    if (relativePath.empty() || m_missing.count(relativePath)) return nullptr;

    // Key: "path\n<overlap>" (a path can't contain a line break)
    const std::string prefix = relativePath + "\n";
    const std::string key = prefix + std::to_string(overlap);
    auto it = m_textures.find(key);
    if (it != m_textures.end()) return it->second.get();

    sf::Image image;
    if (!image.loadFromFile(m_projectFolder / relativePath))
    {
        m_missing.insert(relativePath);
        return nullptr;
    }

    auto texture = std::make_unique<sf::Texture>();
    if (!texture->loadFromImage(ImageTools::make_seamless(image, static_cast<unsigned>(overlap)))) return nullptr;

    // Only the latest overlap of each image is kept
    std::erase_if(m_textures, [&](const auto& entry) { return entry.first.rfind(prefix, 0) == 0; });

    const sf::Texture* result = texture.get();
    m_textures.emplace(key, std::move(texture));
    return result;
}

void TextureCache::forget(const std::string& relativePath)
{
    const std::string prefix = relativePath + "\n";
    std::erase_if(m_textures, [&](const auto& entry) { return entry.first == relativePath || entry.first.rfind(prefix, 0) == 0; });
    m_missing.erase(relativePath);
}

void TextureCache::clear()
{
    m_textures.clear();
    m_missing.clear();
}
