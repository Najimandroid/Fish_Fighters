#include "UIInfoPanel.h"
#include "../UISkin.h"
#include "SFML/Window/Event.hpp"

#include <algorithm>
#include <cmath>

namespace
{
    const float WIDTH = 1180.f;
    const float PADDING = 64.f;
    const float TEXT_TOP = UIPopupFrame::HEADER + 36.f;   // Under the header band (the title is on it)
    const float MAX_HEIGHT = 900.f;                        // Taller text scrolls
    const unsigned int TEXT_SIZE = 32;
    const unsigned int TITLE_SIZE = 36;
    const float LINE_HEIGHT = 42.f;
    const float TITLE_HEIGHT = 50.f;
    const float SECTION_GAP = 22.f;                        // Above a title (not the first)
    const float PARAGRAPH_GAP = 18.f;                      // An empty line
    const float BULLET_INDENT = 40.f;
    const float SCROLLBAR_WIDTH = 10.f;
    const float DRAG_START = 10.f;                         // A finger moving further than this scrolls (not a tap)
    const sf::Color SECTION{ 120, 210, 255 };              // Section titles and bullets: light blue

    float text_width() { return WIDTH - 2.f * PADDING; }

    // {name|words}: the colours the text can use (the types' and abilities' are the ones the game shows them in)
    sf::Color color_named(const std::string& name)
    {
        static const std::pair<const char*, sf::Color> COLORS[] = {
            { "yellow", UISkin::NUMBER }, { "green", { 110, 220, 120 } }, { "red", { 255, 128, 104 } },
            { "blue", { 120, 200, 255 } }, { "cyan", { 110, 230, 230 } }, { "pink", { 255, 150, 210 } },
            { "purple", { 196, 140, 255 } }, { "orange", { 255, 170, 90 } }, { "gray", { 180, 190, 205 } },
            { "white", UISkin::TEXT }, { "rare", { 255, 204, 80 } }, { "superrare", { 255, 128, 214 } },
            { "crustacean", { 255, 150, 80 } }, { "shelled", { 196, 140, 255 } }, { "jelly", { 255, 130, 214 } },
            { "strong", { 255, 110, 90 } }, { "resistant", { 120, 185, 255 } }, { "weaken", { 210, 160, 255 } },
            { "paralyze", { 255, 226, 80 } }, { "slow", { 130, 230, 255 } }, { "thorns", { 150, 230, 110 } } };
        for (const auto& [key, color] : COLORS)
            if (name == key) return color;
        return UISkin::NUMBER;
    }
}

/*
 * The markup laid out in lines: titles, bullets, and words wrapped to the width, each line cut into runs of one
 * style: body text (regular font), or coloured words (*yellow*, {name|...}: the heavy font in that colour)
 */
UIInfoPanel::Layout UIInfoPanel::lay_out(const std::string& text)
{
    Layout layout;
    std::vector<sf::Text>& runs = layout.runs;
    float y = 0.f;
    bool isFirstLine = true;

    // A style: none (body text) or a colour (heavy font)
    struct Style { bool isColored = false; sf::Color color; bool operator==(const Style& o) const { return isColored == o.isColored && (!isColored || color == o.color); } };

    // How far the pen goes writing s (its advance, not its ink: the next word starts there)
    const auto measure = [](const std::string& s, const Style& style) {
        sf::Text t(style.isColored ? UISkin::heavy_font() : UISkin::regular_font(), sf::String::fromUtf8(s.begin(), s.end()), TEXT_SIZE);
        return t.findCharacterPos(t.getString().getSize()).x;
    };
    const auto end_of = [](const sf::Text& run) { return run.findCharacterPos(run.getString().getSize()).x; };
    const float spaceWidth = measure(" ", Style{});

    std::vector<std::string> lines;
    size_t start = 0;
    while (true)
    {
        const size_t end = text.find('\n', start);
        lines.push_back(text.substr(start, end == std::string::npos ? std::string::npos : end - start));
        if (end == std::string::npos) break;
        start = end + 1;
    }

    for (const std::string& line : lines)
    {
        if (line.empty())
        {
            if (!isFirstLine) y += PARAGRAPH_GAP;
            continue;
        }
        if (line.rfind("# ", 0) == 0)
        {
            if (!isFirstLine) y += SECTION_GAP;
            const std::string titleText = line.substr(2);
            sf::Text title(UISkin::heavy_font(), sf::String::fromUtf8(titleText.begin(), titleText.end()));
            UISkin::style_text(title, TITLE_SIZE, SECTION);
            title.setPosition({ 0.f, y });
            runs.push_back(title);
            y += TITLE_HEIGHT;
            isFirstLine = false;
            continue;
        }

        // A paragraph line, maybe a bullet: its words wrapped, the coloured ones in their own runs
        std::string body = line;
        float indent = 0.f;
        if (body.rfind("- ", 0) == 0)
        {
            body = body.substr(2);
            indent = BULLET_INDENT;
            layout.bullets.push_back({ BULLET_INDENT / 2.f - 4.f, y + LINE_HEIGHT / 2.f });
        }
        float x = indent;
        int run = -1;                 // The run being filled (-1: a new one is needed)
        Style runStyle;
        const auto new_run = [&](const Style& style) {
            sf::Text t(style.isColored ? UISkin::heavy_font() : UISkin::regular_font());
            if (style.isColored) UISkin::style_text(t, TEXT_SIZE, style.color);
            else UISkin::style_body_text(t, TEXT_SIZE);
            t.setPosition({ x, y + 4.f });
            runs.push_back(t);
            run = static_cast<int>(runs.size()) - 1;
            runStyle = style;
        };
        const auto put_word = [&](const std::string& w, const Style& style) {
            if (w.empty()) return;
            const float width = measure(w, style);
            if (x > indent + 0.5f && x + spaceWidth + width > text_width())
            {
                y += LINE_HEIGHT;   // Wrapped: the next line keeps the bullet's indent
                x = indent;
                run = -1;
            }
            const sf::String word = sf::String::fromUtf8(w.begin(), w.end());
            if (run >= 0 && runStyle == style)
                runs[run].setString(runs[run].getString() + sf::String(" ") + word);
            else
            {
                if (x > indent + 0.5f) x += spaceWidth;
                new_run(style);
                runs[run].setString(word);
            }
            x = end_of(runs[run]);   // Where the run really ends
        };

        Style style;                  // The words being read: body, *yellow*, or {name|...}
        bool isStarred = false, isBraced = false;
        bool justClosed = false;      // A * or } just ended a coloured part (punctuation right after it sticks to it)
        std::string word;
        for (size_t i = 0; i <= body.size(); ++i)
        {
            const char c = i < body.size() ? body[i] : ' ';
            if (c == '*' && !isBraced)
            {
                put_word(word, style);
                word.clear();
                isStarred = !isStarred;
                style = isStarred ? Style{ true, UISkin::NUMBER } : Style{};
                justClosed = !isStarred;
                continue;
            }
            if (c == '{' && !isStarred)
            {
                const size_t bar = body.find('|', i), close = body.find('}', i);
                if (bar != std::string::npos && close != std::string::npos && bar < close)
                {
                    put_word(word, style);
                    word.clear();
                    isBraced = true;
                    style = Style{ true, color_named(body.substr(i + 1, bar - i - 1)) };
                    i = bar;   // Go on after the |
                    justClosed = false;
                    continue;
                }
            }
            if (c == '}' && isBraced)
            {
                put_word(word, style);
                word.clear();
                isBraced = false;
                style = Style{};
                justClosed = true;
                continue;
            }
            if (c == ' ')
            {
                put_word(word, style);
                word.clear();
                justClosed = false;
                continue;
            }
            // Punctuation right after a coloured part: glued to it (no space), in body text
            if (word.empty() && run >= 0 && justClosed && std::string(",.:;)!?").find(c) != std::string::npos)
            {
                const std::string mark(1, c);
                if (runStyle.isColored)
                {
                    new_run(Style{});
                    runs[run].setString(mark);
                }
                else runs[run].setString(runs[run].getString() + sf::String(mark));
                x = end_of(runs[run]);
                continue;
            }
            justClosed = false;
            word += c;
        }
        y += LINE_HEIGHT;
        isFirstLine = false;
    }
    layout.height = y;
    return layout;
}

sf::FloatRect UIInfoPanel::box_for(float contentHeight)
{
    const float height = std::min(MAX_HEIGHT, TEXT_TOP + contentHeight + PADDING * 0.75f);
    return { { (1920.f - WIDTH) / 2.f, (1080.f - height) / 2.f }, { WIDTH, height } };
}

UIInfoPanel::UIInfoPanel(const std::string& title, const std::string& text) :
    UIElement({ 1920.f, 1080.f }, { 0.f, 0.f }),
    m_layout(lay_out(text)),
    m_box(box_for(m_layout.height)),
    m_frame(m_box, title)
{
    set_z(Z_WINDOW);
    m_frame.set_on_close([this]() { close(); });
    m_frame.set_face("icon_fish_info");   // The puzzled Fish: a help window
    m_isVisible = false;
}

sf::FloatRect UIInfoPanel::content_rect() const
{
    return { { m_box.position.x + PADDING, m_box.position.y + TEXT_TOP }, { text_width(), m_box.size.y - TEXT_TOP - PADDING * 0.5f } };
}

void UIInfoPanel::open()
{
    m_isVisible = true;
    m_isPressedWhileOpen = false;
    m_isDragging = false;
    m_scroll = 0.f;
}

void UIInfoPanel::close()
{
    m_isVisible = false;
}

void UIInfoPanel::update(float deltaTime)
{
    if (m_isVisible) m_frame.update(deltaTime);
}

void UIInfoPanel::render(sf::RenderWindow& window)
{
    if (!m_isVisible) return;
    sf::RectangleShape dim({ 1920.f, 1080.f });
    dim.setFillColor(sf::Color(0, 0, 0, 150));
    window.draw(dim);
    m_frame.render(window);

    // The text, clipped to its area (it scrolls inside)
    const sf::FloatRect area = content_rect();
    const sf::View view = window.getView();
    const sf::Vector2f windowSize(window.getSize());
    const sf::Vector2i topLeft = window.mapCoordsToPixel(area.position, view);
    const sf::Vector2i bottomRight = window.mapCoordsToPixel(area.position + area.size + sf::Vector2f{ 0.f, 6.f }, view);
    sf::View clipped = view;
    const sf::FloatRect scissor{ { std::clamp(topLeft.x / windowSize.x, 0.f, 1.f), std::clamp(topLeft.y / windowSize.y, 0.f, 1.f) },
        { std::clamp((bottomRight.x - topLeft.x) / windowSize.x, 0.f, 1.f), std::clamp((bottomRight.y - topLeft.y) / windowSize.y, 0.f, 1.f) } };
    if (scissor.size.x > 0.f && scissor.size.y > 0.f) clipped.setScissor(scissor);
    window.setView(clipped);

    sf::RenderStates states;
    states.transform.translate(area.position - sf::Vector2f{ 0.f, std::round(m_scroll) });
    for (const sf::Text& run : m_layout.runs) window.draw(run, states);
    sf::CircleShape bullet(7.f);
    bullet.setOrigin({ 7.f, 7.f });
    bullet.setFillColor(SECTION);
    bullet.setOutlineThickness(3.f);
    bullet.setOutlineColor(UISkin::OUTLINE);
    for (const sf::Vector2f& center : m_layout.bullets)
    {
        bullet.setPosition(center);
        window.draw(bullet, states);
    }
    window.setView(view);

    // Scrolling: a bar on the right shows where the visible part is
    const float hidden = m_layout.height - area.size.y;
    if (hidden > 1.f)
    {
        const float trackTop = area.position.y, trackHeight = area.size.y;
        const float thumbHeight = std::max(60.f, trackHeight * area.size.y / m_layout.height);
        const float thumbTop = trackTop + (trackHeight - thumbHeight) * (m_scroll / hidden);
        const float x = m_box.position.x + m_box.size.x - PADDING / 2.f - SCROLLBAR_WIDTH / 2.f;
        sf::RectangleShape track({ SCROLLBAR_WIDTH, trackHeight });
        track.setPosition({ x, trackTop });
        track.setFillColor(sf::Color(255, 255, 255, 40));
        window.draw(track);
        sf::RectangleShape thumb({ SCROLLBAR_WIDTH, thumbHeight });
        thumb.setPosition({ x, thumbTop });
        thumb.setFillColor(UISkin::NUMBER);
        window.draw(thumb);
    }
}

void UIInfoPanel::handle_event(const sf::Event& event, const sf::RenderWindow& window)
{
    if (!m_isVisible) return;
    m_frame.handle_event(event, window);
    if (!m_isVisible) return;

    const float hidden = std::max(0.f, m_layout.height - content_rect().size.y);
    if (const auto* key = event.getIf<sf::Event::KeyPressed>())
    {
        if (key->code == sf::Keyboard::Key::Escape || key->code == sf::Keyboard::Key::Enter)
        {
            play_sound("ui_back");
            close();
        }
        else if (key->code == sf::Keyboard::Key::Down) m_scroll = std::min(hidden, m_scroll + LINE_HEIGHT * 2.f);
        else if (key->code == sf::Keyboard::Key::Up) m_scroll = std::max(0.f, m_scroll - LINE_HEIGHT * 2.f);
    }
    else if (const auto* wheel = event.getIf<sf::Event::MouseWheelScrolled>())
    {
        m_scroll = std::clamp(m_scroll - wheel->delta * LINE_HEIGHT * 2.f, 0.f, hidden);
    }
    else if (const auto* pressed = event.getIf<sf::Event::MouseButtonPressed>())
    {
        // Anywhere: a finger that then moves scrolls the text (it may start or end outside the panel), one that
        // doesn't is a tap
        m_isPressedWhileOpen = true;
        const sf::Vector2f point = window.mapPixelToCoords(pressed->position);
        m_isDragging = true;
        m_hasDragged = false;
        m_dragStartY = point.y;
        m_dragStartScroll = m_scroll;
        m_isPressOutside = !m_box.contains(point);
    }
    else if (const auto* moved = event.getIf<sf::Event::MouseMoved>(); moved && m_isDragging)
    {
        const float dragY = window.mapPixelToCoords(moved->position).y - m_dragStartY;
        if (std::abs(dragY) > DRAG_START) m_hasDragged = true;
        if (m_hasDragged && hidden > 0.f) m_scroll = std::clamp(m_dragStartScroll - dragY, 0.f, hidden);
    }
    else if (const auto* released = event.getIf<sf::Event::MouseButtonReleased>())
    {
        // Only a tap outside the panel closes it (a phone has no Escape): not a slide, wherever it ends, and only a
        // tap begun while it's open (the release of the tap on the "i" that opened it reaches it too)
        const sf::Vector2f point = window.mapPixelToCoords(released->position);
        const bool wasTap = m_isPressedWhileOpen && !m_hasDragged && m_isPressOutside;
        m_isDragging = false;
        m_hasDragged = false;
        m_isPressedWhileOpen = false;
        if (wasTap && released->button == sf::Mouse::Button::Left && !m_box.contains(point) && !m_frame.get_close_bounds().contains(point))
        {
            play_sound("ui_back");
            close();
        }
    }
}

sf::FloatRect UIInfoPanel::get_bounds() const
{
    return m_isVisible ? sf::FloatRect{ { 0.f, 0.f }, { 1920.f, 1080.f } } : sf::FloatRect{};
}

bool UIInfoPanel::is_blocking_input() const
{
    return m_isVisible;
}
