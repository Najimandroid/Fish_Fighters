#include "UISkin.h"

#include <algorithm>
#include <cmath>
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

const UISkin::SwimFrames& UISkin::swim_frames(const std::string& sheetPath, int frameCount, int knockbackFrameIndex)
{
    static std::unordered_map<std::string, SwimFrames> cache;
    if (auto it = cache.find(sheetPath); it != cache.end()) return it->second;

    SwimFrames frames;
    const sf::Texture& sheet = texture_file(sheetPath);
    const sf::Vector2u size = sheet.getSize();
    const unsigned int frameWidth = size.x / static_cast<unsigned int>(std::max(1, frameCount));
    frames.frameWidth = static_cast<int>(frameWidth);
    frames.count = std::clamp(knockbackFrameIndex, 1, std::max(1, frameCount));
    const sf::Image image = sheet.copyToImage();
    unsigned int left = frameWidth, top = size.y, right = 0, bottom = 0;
    for (int frame = 0; frame < frames.count; ++frame)
        for (unsigned int y = 0; y < size.y; ++y)
            for (unsigned int x = 0; x < frameWidth; ++x)
                if (image.getPixel({ static_cast<unsigned int>(frame) * frameWidth + x, y }).a > 24)
                {
                    left = std::min(left, x); right = std::max(right, x);
                    top = std::min(top, y); bottom = std::max(bottom, y);
                }
    if (left > right || top > bottom) frames.box = { { 0, 0 }, { static_cast<int>(frameWidth), static_cast<int>(size.y) } };
    else frames.box = { { static_cast<int>(left), static_cast<int>(top) }, { static_cast<int>(right - left + 1), static_cast<int>(bottom - top + 1) } };
    return cache.emplace(sheetPath, frames).first->second;
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

/*
 * The transitions' band of deep water: a triangle strip between two wavy edges, a foam line along each, and a few
 * bubbles rising inside (only where they fit whole)
 */
namespace
{
    sf::FloatRect g_screen{ { 0.f, 0.f }, { 1920.f, 1080.f } };
}

sf::FloatRect UISkin::screen_rect() { return g_screen; }
void UISkin::set_screen_rect(const sf::FloatRect& rect) { g_screen = rect; }

void UISkin::fill_screen(sf::RenderTarget& target, sf::Color color)
{
    sf::RectangleShape fill(g_screen.size);
    fill.setPosition(g_screen.position);
    fill.setFillColor(color);
    target.draw(fill);
}

bool UISkin::is_slide_area(sf::Vector2f point)
{
    const sf::Vector2f margin{ 16.f, 16.f };   // A finger is wider than the mouse: a little room round the button
    const sf::FloatRect back{ BACK_BUTTON.position - margin, BACK_BUTTON.size + margin * 2.f };
    return point.y > CONTENT_TOP && !back.contains(point);
}

/*
 * Hull pillars on both sides of the 16:9 layout: darker outside, a bevel and a dark edge on the inside, a column of
 * rivets, and a shadow they cast on the screen between them
 */
sf::ConvexShape UISkin::rounded_rect(const sf::FloatRect& rect, float radius)
{
    radius = std::min({ radius, rect.size.x / 2.f, rect.size.y / 2.f });
    const int perCorner = 8;
    sf::ConvexShape shape(perCorner * 4);
    const sf::Vector2f centers[] = {
        { rect.position.x + rect.size.x - radius, rect.position.y + radius },
        { rect.position.x + rect.size.x - radius, rect.position.y + rect.size.y - radius },
        { rect.position.x + radius, rect.position.y + rect.size.y - radius },
        { rect.position.x + radius, rect.position.y + radius } };
    for (int corner = 0; corner < 4; ++corner)
        for (int i = 0; i < perCorner; ++i)
        {
            const float angle = (-90.f + 90.f * static_cast<float>(corner) + 90.f * static_cast<float>(i) / (perCorner - 1)) * 3.14159265f / 180.f;
            shape.setPoint(static_cast<std::size_t>(corner * perCorner + i), centers[corner] + sf::Vector2f{ std::cos(angle), std::sin(angle) } * radius);
        }
    return shape;
}

void UISkin::draw_side_frame(sf::RenderTarget& target)
{
    const float side = -g_screen.position.x;   // The extra width on each side
    if (side < 4.f) return;
    const float top = g_screen.position.y, bottom = top + g_screen.size.y;
    const sf::Color outer{ 30, 40, 56 }, inner{ 66, 84, 108 }, bevel{ 128, 148, 172 };

    for (const bool isRight : { false, true })
    {
        const float outerX = isRight ? 1920.f + side : -side, innerX = isRight ? 1920.f : 0.f;
        const float toInner = isRight ? -1.f : 1.f;   // Towards the layout

        sf::VertexArray body(sf::PrimitiveType::TriangleStrip, 4);
        body[0] = sf::Vertex{ { outerX, top }, outer };
        body[1] = sf::Vertex{ { innerX, top }, inner };
        body[2] = sf::Vertex{ { outerX, bottom }, outer };
        body[3] = sf::Vertex{ { innerX, bottom }, inner };
        target.draw(body);

        // The bevel, then the dark edge, then the shadow on the layout
        sf::RectangleShape line({ 4.f, bottom - top });
        line.setFillColor(bevel);
        line.setPosition({ innerX - toInner * 14.f - (isRight ? 0.f : 4.f), top });
        target.draw(line);
        line.setSize({ 6.f, bottom - top });
        line.setFillColor(OUTLINE);
        line.setPosition({ isRight ? innerX : innerX - 6.f, top });
        target.draw(line);
        sf::VertexArray shadow(sf::PrimitiveType::TriangleStrip, 4);
        const float shadowEnd = innerX + toInner * 28.f;
        shadow[0] = sf::Vertex{ { innerX, top }, sf::Color(0, 0, 0, 110) };
        shadow[1] = sf::Vertex{ { shadowEnd, top }, sf::Color(0, 0, 0, 0) };
        shadow[2] = sf::Vertex{ { innerX, bottom }, sf::Color(0, 0, 0, 110) };
        shadow[3] = sf::Vertex{ { shadowEnd, bottom }, sf::Color(0, 0, 0, 0) };
        target.draw(shadow);

        // Rivets down the pillar
        const float rivetX = innerX - toInner * std::min(46.f, side * 0.4f);
        sf::CircleShape rivet(8.f);
        rivet.setOrigin({ 8.f, 8.f });
        rivet.setOutlineThickness(3.f);
        rivet.setOutlineColor(OUTLINE);
        for (float y = top + 70.f; y < bottom - 30.f; y += 150.f)
        {
            rivet.setFillColor(sf::Color(112, 130, 154));
            rivet.setPosition({ rivetX, y });
            target.draw(rivet);
        }
    }
}

void UISkin::draw_water_band(sf::RenderTarget& target, float top, float bottom, float time)
{
    if (bottom - top < 1.f) return;
    // Across the whole screen, a phone's extra width too (and down past a tablet's extra height)
    const float left = g_screen.position.x, width = g_screen.size.x;
    top = std::max(top, g_screen.position.y - WATER_WAVE * 3.f);
    bottom = std::min(bottom, g_screen.position.y + g_screen.size.y + WATER_WAVE * 3.f);
    if (bottom - top < 1.f) return;
    const sf::Color upper{ 38, 104, 156 }, lower{ 14, 46, 86 };
    const sf::Color foam{ 196, 236, 255, 210 };
    const float step = 32.f, foamWidth = 7.f;
    const auto top_edge = [&](float x) { return top + std::sin(x * 0.011f + time * 7.f) * WATER_WAVE * 0.7f + std::sin(x * 0.027f - time * 4.f) * WATER_WAVE * 0.3f; };
    const auto bottom_edge = [&](float x) { return bottom + std::sin(x * 0.012f - time * 6.f + 1.3f) * WATER_WAVE * 0.7f + std::sin(x * 0.023f + time * 5.f) * WATER_WAVE * 0.3f; };

    const int columns = static_cast<int>(width / step) + 2;
    sf::VertexArray water(sf::PrimitiveType::TriangleStrip), topFoam(sf::PrimitiveType::TriangleStrip), bottomFoam(sf::PrimitiveType::TriangleStrip);
    for (int i = 0; i <= columns; ++i)
    {
        const float x = left - step + static_cast<float>(i) * step;
        const float a = top_edge(x), b = bottom_edge(x);
        water.append({ { x, a }, upper });
        water.append({ { x, b }, lower });
        topFoam.append({ { x, a }, foam });
        topFoam.append({ { x, a + foamWidth }, sf::Color(foam.r, foam.g, foam.b, 0) });
        bottomFoam.append({ { x, b - foamWidth }, sf::Color(foam.r, foam.g, foam.b, 0) });
        bottomFoam.append({ { x, b }, foam });
    }
    target.draw(water);
    target.draw(topFoam);
    target.draw(bottomFoam);

    // A few bubbles rising through it (always the same ones, so they don't flicker)
    const sf::Texture& bubble = texture("bubble");
    const float textureSize = static_cast<float>(std::max(1u, bubble.getSize().x));
    sf::Sprite sprite(bubble);
    const float height = bottom - top;
    for (int i = 0; i < 12; ++i)
    {
        const float size = 34.f + static_cast<float>((i * 37) % 50);
        const float x = left + std::fmod(static_cast<float>(i * 389 + 120), width) + std::sin(time * 3.f + i) * 10.f;
        const float speed = 260.f + static_cast<float>((i * 53) % 140);
        const float y = bottom - std::fmod(time * speed + static_cast<float>(i) * 131.f, std::max(1.f, height));
        if (y - size < top + WATER_WAVE + 8.f || y > bottom - WATER_WAVE - 8.f) continue;
        sprite.setScale({ size / textureSize, size / textureSize });
        sprite.setPosition({ x - size / 2.f, y - size });
        sprite.setColor(sf::Color(255, 255, 255, 130));
        target.draw(sprite);
    }
}
