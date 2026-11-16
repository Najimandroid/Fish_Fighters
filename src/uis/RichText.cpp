#include "RichText.h"
#include "UISkin.h"

#include <algorithm>
#include <cmath>

namespace
{
    // The currencies a text can show as icons: [token] -> the counters' texture (UISkin::texture)
    struct Token { const char* name; const char* texture; };
    constexpr Token TOKENS[] = {
        { "shell", "icon_shell" }, { "pearl", "icon_pearl" }, { "shiny", "icon_shiny_scale" },
        { "scale", "icon_fish_scale" }, { "energy", "icon_energy" } };
    constexpr std::size_t LONGEST_TOKEN = 8;   // "[energy]" without its brackets, with room

    // A piece of a word: letters, or an icon
    struct Piece { sf::String text; const char* icon = nullptr; };
    // A word: the pieces between two spaces ("20", "[pearl]." is an icon and its full stop: they stay together)
    struct Word { std::vector<Piece> pieces; float width = 0.f; };
    struct Line { std::vector<const Word*> words; float width = 0.f; };
}

const char* RichText::icon_texture(std::string_view token)
{
    for (const Token& known : TOKENS)
        if (token == known.name) return known.texture;
    return nullptr;
}

float RichText::icon_size(unsigned int characterSize)
{
    return static_cast<float>(characterSize) * ICON_SCALE;
}

float RichText::icon_center_y(const sf::Font& font, unsigned int characterSize)
{
    // Between the capitals' top and the baseline (SFML puts the first baseline at the character size)
    const float size = static_cast<float>(characterSize);
    const float capitalTop = size + font.getGlyph(U'H', characterSize, false).bounds.position.y;
    return (capitalTop + size) / 2.f;
}

void RichText::draw_icon(sf::RenderTarget& target, const char* textureName, sf::Vector2f center, float size,
    sf::RenderStates states, std::uint8_t alpha)
{
    const sf::Texture& texture = UISkin::texture(textureName);
    const sf::Vector2f textureSize(texture.getSize());
    if (textureSize.x <= 0.f || textureSize.y <= 0.f) return;
    const float scale = size / std::max(textureSize.x, textureSize.y);
    sf::Sprite sprite(texture);
    sprite.setOrigin(textureSize / 2.f);
    sprite.setScale({ scale, scale });
    sprite.setPosition(center);
    sprite.setColor(sf::Color(255, 255, 255, alpha));
    target.draw(sprite, states);
}

void RichText::draw_text(sf::RenderTarget& target, const sf::Text& style, const std::string& string, sf::Vector2f position,
    sf::Vector2f anchor, float alignment, float maxWidth, sf::RenderStates states)
{
    RichText text(style);
    text.m_alignment = std::clamp(alignment, 0.f, 1.f);
    text.m_maxWidth = std::max(0.f, maxWidth);
    text.set_string(string);
    text.set_position(position, anchor);
    text.draw(target, states);
}

RichText::RichText() :
    m_style(UISkin::heavy_font())
{
    UISkin::style_text(m_style, 30);
}

RichText::RichText(const sf::Text& style) :
    m_style(style)
{
}

void RichText::set_style(const sf::Text& style)
{
    m_style = style;
    lay_out();
}

void RichText::set_fill_color(sf::Color fill)
{
    sf::Color outline = m_style.getOutlineColor();
    outline.a = fill.a;
    m_style.setFillColor(fill);
    m_style.setOutlineColor(outline);
    for (Run& run : m_runs)
    {
        run.text.setFillColor(fill);
        run.text.setOutlineColor(outline);
    }
}

void RichText::set_string(const std::string& text)
{
    if (text == m_string && (!m_runs.empty() || !m_icons.empty() || text.empty())) return;   // Set every frame: laid out once
    m_string = text;
    lay_out();
}

void RichText::set_max_width(float width)
{
    if (width == m_maxWidth) return;
    m_maxWidth = std::max(0.f, width);
    lay_out();
}

void RichText::set_alignment(float alignment)
{
    if (alignment == m_alignment) return;
    m_alignment = std::clamp(alignment, 0.f, 1.f);
    lay_out();
}

void RichText::set_position(sf::Vector2f position, sf::Vector2f anchor)
{
    m_position = position;
    m_anchor = anchor;
}

void RichText::set_revealed(std::size_t count)
{
    m_revealed = count;
}

float RichText::get_line_height() const
{
    return m_style.getFont().getLineSpacing(m_style.getCharacterSize()) * m_style.getLineSpacing();
}

sf::FloatRect RichText::get_bounds() const
{
    const sf::Vector2f origin{ m_size.x * m_anchor.x, m_top + (m_bottom - m_top) * m_anchor.y };
    return { m_position - origin + sf::Vector2f{ 0.f, m_top }, { m_size.x, m_bottom - m_top } };
}

/*
 * Words (cut at spaces, the icons and the letters around them kept together), wrapped into lines no wider than the
 * max width, each line placed by the alignment; the letters between icons go into one sf::Text each (a run)
 */
void RichText::lay_out()
{
    m_runs.clear();
    m_icons.clear();
    m_length = 0;

    const sf::Font& font = m_style.getFont();
    const unsigned int characterSize = m_style.getCharacterSize();
    const float size = static_cast<float>(characterSize);
    sf::Text measurer = m_style;
    measurer.setOrigin({ 0.f, 0.f });
    measurer.setPosition({ 0.f, 0.f });
    const auto advance = [&measurer](const sf::String& s) {
        measurer.setString(s);
        return measurer.findCharacterPos(s.getSize()).x;
    };
    const float spaceWidth = advance(" ");
    const float padding = ICON_PADDING * size;
    const float iconWidth = icon_size(characterSize) + 2.f * padding;

    // The paragraphs (\n) and their words
    const sf::String all = sf::String::fromUtf8(m_string.begin(), m_string.end());
    std::vector<std::vector<Word>> paragraphs(1);
    Word word;
    Piece piece;
    const auto end_piece = [&]() { if (!piece.text.isEmpty()) word.pieces.push_back(piece); piece = Piece{}; };
    const auto end_word = [&]() { end_piece(); if (!word.pieces.empty()) paragraphs.back().push_back(word); word = Word{}; };
    for (std::size_t i = 0; i < all.getSize(); ++i)
    {
        const char32_t c = all[i];
        if (c == U'\n') { end_word(); paragraphs.emplace_back(); continue; }
        if (c == U' ') { end_word(); continue; }
        if (c == U'[')
        {
            const std::size_t close = all.find(sf::String("]"), i);
            if (close != sf::String::InvalidPos && close - i <= LONGEST_TOKEN + 1)
                if (const char* texture = icon_texture(all.substring(i + 1, close - i - 1).toAnsiString()))
                {
                    end_piece();
                    word.pieces.push_back(Piece{ {}, texture });
                    i = close;
                    continue;
                }
        }
        piece.text += c;
    }
    end_word();
    for (auto& words : paragraphs)
        for (Word& w : words)
            for (const Piece& p : w.pieces) w.width += p.icon ? iconWidth : advance(p.text);

    // Lines: a word goes on the next line when it would pass the max width (a word wider than it gets a line alone)
    std::vector<Line> lines;
    for (const auto& words : paragraphs)
    {
        lines.emplace_back();
        for (const Word& w : words)
        {
            Line* line = &lines.back();
            if (!line->words.empty() && m_maxWidth > 0.f && line->width + spaceWidth + w.width > m_maxWidth)
            {
                lines.emplace_back();
                line = &lines.back();
            }
            line->width += (line->words.empty() ? 0.f : spaceWidth) + w.width;
            line->words.push_back(&w);
        }
    }
    float widest = 0.f;
    for (const Line& line : lines) widest = std::max(widest, line.width);

    // Runs and icons, each line placed in the block's width by the alignment
    const float lineHeight = get_line_height();
    const float iconY = icon_center_y(font, characterSize);
    std::size_t index = 0;   // Characters so far (an icon and a space are one each)
    for (std::size_t l = 0; l < lines.size(); ++l)
    {
        const float y = static_cast<float>(l) * lineHeight;
        float x = std::round((widest - lines[l].width) * m_alignment);
        int open = -1;        // The run being filled (-1: none, the next letters start one)
        float runX = 0.f;     // Where it starts
        const auto append = [&](const sf::String& s) {
            if (open < 0)
            {
                Run run{ m_style, index };
                run.text.setString("");
                run.text.setOrigin({ 0.f, 0.f });
                run.text.setPosition({ x, y });
                m_runs.push_back(run);
                open = static_cast<int>(m_runs.size()) - 1;
                runX = x;
            }
            sf::Text& text = m_runs[static_cast<std::size_t>(open)].text;
            text.setString(text.getString() + s);
            x = runX + advance(text.getString());
            index += s.getSize();
        };
        for (std::size_t w = 0; w < lines[l].words.size(); ++w)
        {
            if (w > 0)
            {
                if (open >= 0) append(" ");
                else { x += spaceWidth; ++index; }
            }
            for (const Piece& p : lines[l].words[w]->pieces)
            {
                if (!p.icon) { append(p.text); continue; }
                open = -1;
                m_icons.push_back({ p.icon, { x + iconWidth / 2.f, y + iconY }, index });
                x += iconWidth;
                ++index;
            }
        }
    }
    m_length = index;
    m_lineCount = std::max<std::size_t>(1, lines.size());

    // The block: from the capitals' top to the last baseline, like UISkin::set_text_anchor
    const float outline = m_style.getOutlineThickness();
    m_top = size + font.getGlyph(U'H', characterSize, false, outline).bounds.position.y;
    m_bottom = size + static_cast<float>(m_lineCount - 1) * lineHeight + outline;
    m_size = { widest, static_cast<float>(m_lineCount) * lineHeight };
}

void RichText::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
    const sf::Vector2f origin{ m_size.x * m_anchor.x, m_top + (m_bottom - m_top) * m_anchor.y };
    const sf::Vector2f topLeft = m_position - origin;
    states.transform.translate({ std::round(topLeft.x), std::round(topLeft.y) });

    for (const Run& run : m_runs)
    {
        if (run.firstChar >= m_revealed) continue;
        const std::size_t count = run.text.getString().getSize();
        if (m_revealed != std::string::npos && m_revealed - run.firstChar < count)
        {
            sf::Text part = run.text;
            part.setString(run.text.getString().substring(0, m_revealed - run.firstChar));
            target.draw(part, states);
        }
        else target.draw(run.text, states);
    }
    const float size = icon_size(m_style.getCharacterSize());
    for (const Icon& icon : m_icons)
        if (icon.index < m_revealed) draw_icon(target, icon.texture, icon.center, size, states, m_style.getFillColor().a);
}
