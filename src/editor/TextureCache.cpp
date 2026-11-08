#include "TextureCache.h"

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

void TextureCache::forget(const std::string& relativePath)
{
    m_textures.erase(relativePath);
    m_missing.erase(relativePath);
}

void TextureCache::clear()
{
    m_textures.clear();
    m_missing.clear();
}
