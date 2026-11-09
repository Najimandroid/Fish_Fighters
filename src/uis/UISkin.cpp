#include "UISkin.h"

#include <algorithm>
#include <iostream>
#include <memory>
#include <unordered_map>

namespace
{
    const std::string FONT_FOLDER = "assets/fonts/";
    const std::string TEXTURE_FOLDER = "assets/images/textures/ui/";

    // Loads a font the first time it's asked for
    const sf::Font& load_font(sf::Font& font, bool& isLoaded, const std::string& fileName)
    {
        if (!isLoaded)
        {
            if (!font.openFromFile(FONT_FOLDER + fileName))
                std::cerr << "UISkin: can't load the font " << fileName << "\n";
            isLoaded = true;
        }
        return font;
    }
}

const sf::Font& UISkin::heavy_font()
{
    static sf::Font font;
    static bool isLoaded = false;
    return load_font(font, isLoaded, "MPLUSRounded1c-Black.ttf");
}

const sf::Font& UISkin::regular_font()
{
    static sf::Font font;
    static bool isLoaded = false;
    return load_font(font, isLoaded, "MPLUSRounded1c-Medium.ttf");
}

const sf::Texture& UISkin::texture(const std::string& name)
{
    // Textures are kept behind pointers so their addresses never change (sprites point to them)
    static std::unordered_map<std::string, std::unique_ptr<sf::Texture>> textures;

    auto it = textures.find(name);
    if (it != textures.end()) return *it->second;

    auto texture = std::make_unique<sf::Texture>();
    if (!texture->loadFromFile(TEXTURE_FOLDER + name + ".png"))
        std::cerr << "UISkin: can't load the texture " << name << "\n";
    texture->setSmooth(true);

    return *textures.emplace(name, std::move(texture)).first->second;
}

void UISkin::draw_nine_slice(sf::RenderTarget& target, const NineSlice& slice, const sf::FloatRect& rect,
                             float cornerScale, sf::Color tint)
{
    const sf::Texture& image = texture(slice.texture);
    const sf::Vector2f imageSize(image.getSize());
    const float margin = slice.margin;

    // Corners on screen, at most half the rect so they never overlap
    const float cornerX = std::min(margin * cornerScale, rect.size.x / 2.f);
    const float cornerY = std::min(margin * cornerScale, rect.size.y / 2.f);

    const float left = rect.position.x;
    const float top = rect.position.y;
    const float right = left + rect.size.x;
    const float bottom = top + rect.size.y;

    // Column and row lines, on screen and in the image
    const float xs[4] = { left, left + cornerX, right - cornerX, right };
    const float ys[4] = { top, top + cornerY, bottom - cornerY, bottom };
    const float us[4] = { 0.f, margin, imageSize.x - margin, imageSize.x };
    const float vs[4] = { 0.f, margin, imageSize.y - margin, imageSize.y };

    sf::VertexArray vertices(sf::PrimitiveType::Triangles);
    for (int row = 0; row < 3; ++row)
    {
        for (int col = 0; col < 3; ++col)
        {
            const sf::Vertex topLeft{ { xs[col], ys[row] }, tint, { us[col], vs[row] } };
            const sf::Vertex topRight{ { xs[col + 1], ys[row] }, tint, { us[col + 1], vs[row] } };
            const sf::Vertex bottomLeft{ { xs[col], ys[row + 1] }, tint, { us[col], vs[row + 1] } };
            const sf::Vertex bottomRight{ { xs[col + 1], ys[row + 1] }, tint, { us[col + 1], vs[row + 1] } };

            vertices.append(topLeft);
            vertices.append(topRight);
            vertices.append(bottomLeft);
            vertices.append(bottomLeft);
            vertices.append(topRight);
            vertices.append(bottomRight);
        }
    }

    sf::RenderStates states;
    states.texture = &image;
    target.draw(vertices, states);
}

void UISkin::style_text(sf::Text& text, unsigned int size, sf::Color fill)
{
    text.setFont(heavy_font());
    text.setCharacterSize(size);
    text.setOutlineThickness(std::max(2.f, static_cast<float>(size) * 0.12f));
    set_text_color(text, fill);
}

void UISkin::set_text_color(sf::Text& text, sf::Color fill)
{
    text.setFillColor(fill);
    sf::Color outline = OUTLINE;
    outline.a = fill.a;
    text.setOutlineColor(outline);
}

void UISkin::set_text_anchor(sf::Text& text, sf::Vector2f anchor)
{
    const sf::FloatRect bounds = text.getLocalBounds();
    text.setOrigin({
        bounds.position.x + bounds.size.x * anchor.x,
        bounds.position.y + bounds.size.y * anchor.y
        });
}
