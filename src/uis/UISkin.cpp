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

/*
 * Scratches and stains (grime.png, repeated) over rect, inset so they stay off the rounded corners.
 * The pattern is pinned to the screen: it never stretches and differs from one panel to the next.
 */
void UISkin::draw_grime(sf::RenderTarget& target, const sf::FloatRect& rect, float inset, sf::Color tint)
{
    static sf::Texture grime;
    static bool isLoaded = false;
    if (!isLoaded)
    {
        if (!grime.loadFromFile(TEXTURE_FOLDER + "grime.png"))
            std::cerr << "UISkin: can't load the texture grime\n";
        grime.setRepeated(true);
        grime.setSmooth(true);
        isLoaded = true;
    }

    const sf::Vector2f size = rect.size - sf::Vector2f{ inset, inset } * 2.f;
    if (size.x < 1.f || size.y < 1.f) return;
    const sf::Vector2f position = rect.position + sf::Vector2f{ inset, inset };

    sf::Sprite sprite(grime, sf::IntRect(sf::Vector2i(position), sf::Vector2i(size)));
    sprite.setPosition(position);
    sprite.setColor(tint);
    target.draw(sprite);
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
    auto addQuad = [&](float x0, float x1, float u0, float u1, int row)
        {
            const sf::Vertex topLeft{ { x0, ys[row] }, tint, { u0, vs[row] } };
            const sf::Vertex topRight{ { x1, ys[row] }, tint, { u1, vs[row] } };
            const sf::Vertex bottomLeft{ { x0, ys[row + 1] }, tint, { u0, vs[row + 1] } };
            const sf::Vertex bottomRight{ { x1, ys[row + 1] }, tint, { u1, vs[row + 1] } };

            vertices.append(topLeft);
            vertices.append(topRight);
            vertices.append(bottomLeft);
            vertices.append(bottomLeft);
            vertices.append(topRight);
            vertices.append(bottomRight);
        };

    // Width on screen of one copy of the middle of the image, when it repeats
    const float middleWidth = us[2] - us[1];
    const float repeatWidth = middleWidth * (cornerY / margin);

    for (int row = 0; row < 3; ++row)
    {
        for (int col = 0; col < 3; ++col)
        {
            if (col == 1 && slice.repeatsX && repeatWidth > 1.f)
            {
                // Copies of the middle side by side, the last one cut
                for (float x = xs[1]; x < xs[2] - 0.01f; x += repeatWidth)
                {
                    const float end = std::min(x + repeatWidth, xs[2]);
                    addQuad(x, end, us[1], us[1] + middleWidth * (end - x) / repeatWidth, row);
                }
            }
            else
            {
                addQuad(xs[col], xs[col + 1], us[col], us[col + 1], row);
            }
        }
    }

    sf::RenderStates states;
    states.texture = &image;
    target.draw(vertices, states);

    // Grime over opaque parts only (not over shades drawn with this image)
    if (slice.hasGrime && tint.a == 255)
        draw_grime(target, rect, std::max(cornerX, cornerY) * 0.45f, tint);
}

void UISkin::style_text(sf::Text& text, unsigned int size, sf::Color fill, const sf::Font& font)
{
    text.setFont(font);
    text.setCharacterSize(size);
    // The thin strokes of the regular font would drown in the heavy outline
    const float outline = &font == &heavy_font() ? 0.12f : 0.08f;
    text.setOutlineThickness(std::max(2.f, static_cast<float>(size) * outline));
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

    // Vertically by the font's lines, not by the letters: from the top of the capitals to the last baseline.
    // "Range" and "Attack" line up (a "g" or a "k" doesn't move the text), and a centered label sits in the
    // middle of its button. (SFML puts the first baseline at the character size, glyphs are placed from it.)
    const sf::Font& font = text.getFont();
    const unsigned int size = text.getCharacterSize();
    const float outline = text.getOutlineThickness();
    const sf::String& string = text.getString();
    const auto lineCount = 1 + std::count(string.begin(), string.end(), U'\n');
    const float lineHeight = font.getLineSpacing(size) * text.getLineSpacing();
    const float top = static_cast<float>(size) + font.getGlyph(U'H', size, false, outline).bounds.position.y;
    const float bottom = static_cast<float>(size) + static_cast<float>(lineCount - 1) * lineHeight + outline;

    text.setOrigin({
        bounds.position.x + bounds.size.x * anchor.x,
        top + (bottom - top) * anchor.y
        });
}

std::string UISkin::wrap_text(const std::string& text, unsigned int size, float maxWidth, const sf::Font& font)
{
    sf::Text measure(font, "", size);
    auto width = [&](const std::string& line) { measure.setString(line); return measure.getLocalBounds().size.x; };

    std::string result;
    std::string line;
    std::string word;
    auto addWord = [&]()
        {
            if (word.empty()) return;
            const std::string candidate = line.empty() ? word : line + " " + word;
            if (!line.empty() && width(candidate) > maxWidth)
            {
                result += line + "\n";
                line = word;
            }
            else
            {
                line = candidate;
            }
            word.clear();
        };

    for (char c : text)
    {
        if (c == ' ') addWord();
        else if (c == '\n') { addWord(); result += line + "\n"; line.clear(); }
        else word += c;
    }
    addWord();
    return result + line;
}

void UISkin::draw_cropped(sf::RenderTarget& target, const sf::Texture& texture, const sf::FloatRect& area, sf::Color tint)
{
    draw_cropped(target, texture, sf::IntRect({ 0, 0 }, sf::Vector2i(texture.getSize())), area, tint);
}

void UISkin::draw_cropped(sf::RenderTarget& target, const sf::Texture& texture, const sf::IntRect& source, const sf::FloatRect& area, sf::Color tint)
{
    const sf::Vector2f sourceSize(source.size);
    if (sourceSize.x <= 0.f || sourceSize.y <= 0.f || area.size.x <= 0.f || area.size.y <= 0.f) return;

    // Biggest centered part of the source with the shape of area
    const float areaRatio = area.size.x / area.size.y;
    sf::Vector2f cropSize = sourceSize;
    if (sourceSize.x / sourceSize.y > areaRatio) cropSize.x = sourceSize.y * areaRatio;
    else cropSize.y = sourceSize.x / areaRatio;
    const sf::Vector2f cropPosition = sf::Vector2f(source.position) + (sourceSize - cropSize) / 2.f;

    sf::Sprite sprite(texture, sf::IntRect(sf::Vector2i(cropPosition), sf::Vector2i(cropSize)));
    sprite.setScale({ area.size.x / cropSize.x, area.size.y / cropSize.y });
    sprite.setPosition(area.position);
    sprite.setColor(tint);
    target.draw(sprite);
}

void UISkin::draw_fitted(sf::RenderTarget& target, const sf::Texture& texture, const sf::FloatRect& area, sf::Color tint)
{
    draw_fitted(target, texture, sf::IntRect({ 0, 0 }, sf::Vector2i(texture.getSize())), area, tint);
}

void UISkin::draw_fitted(sf::RenderTarget& target, const sf::Texture& texture, const sf::IntRect& source, const sf::FloatRect& area, sf::Color tint)
{
    const sf::Vector2f sourceSize(source.size);
    if (sourceSize.x <= 0.f || sourceSize.y <= 0.f) return;

    const float scale = std::min(area.size.x / sourceSize.x, area.size.y / sourceSize.y);
    sf::Sprite sprite(texture, source);
    sprite.setScale({ scale, scale });
    sprite.setPosition(area.position + (area.size - sourceSize * scale) / 2.f);
    sprite.setColor(tint);
    target.draw(sprite);
}

const sf::Texture& UISkin::texture_file(const std::string& path)
{
    static std::unordered_map<std::string, std::unique_ptr<sf::Texture>> textures;

    auto it = textures.find(path);
    if (it != textures.end()) return *it->second;

    auto texture = std::make_unique<sf::Texture>();
    if (!texture->loadFromFile(path))
        std::cerr << "UISkin: can't load " << path << "\n";
    texture->setSmooth(true);

    return *textures.emplace(path, std::move(texture)).first->second;
}

sf::IntRect UISkin::icon_cell(const sf::Texture& sheet, int index, int cellSize)
{
    const int columns = std::max(1, static_cast<int>(sheet.getSize().x) / cellSize);
    return { { (index % columns) * cellSize, (index / columns) * cellSize }, { cellSize, cellSize } };
}

sf::FloatRect UISkin::slot_inner(const sf::FloatRect& rect, float cornerScale)
{
    const float inset = SLOT_OUTLINE * cornerScale;
    return { rect.position + sf::Vector2f{ inset, inset }, rect.size - sf::Vector2f{ inset, inset } * 2.f };
}
