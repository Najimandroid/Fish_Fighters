#include "UIPlaylist.h"
#include "../UISkin.h"
#include "../../audio/AudioManager.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <regex>

namespace
{
    const sf::Vector2f SCREEN_SIZE{ 1920.f, 1080.f };
    const sf::Vector2f PANEL_SIZE{ 1180.f, 960.f };
    const sf::Vector2f PANEL_POSITION{ (SCREEN_SIZE.x - PANEL_SIZE.x) / 2.f, (SCREEN_SIZE.y - PANEL_SIZE.y) / 2.f };
    const float FOOTER_HEIGHT = 180.f;
    const float HEADER_ROW = 74.f, SONG_ROW = 66.f;
    const unsigned int HEADER_SIZE = 30, TITLE_SIZE = 30, LENGTH_SIZE = 26;
    const sf::Color LENGTH_COLOR{ 170, 190, 210 };
    const float DRAG_THRESHOLD = 12.f;
    // The controls, on the right of the footer: previous, play or stop, next
    const sf::Vector2f SKIP_SIZE{ 120.f, 100.f }, PLAY_SIZE{ 150.f, 100.f };
    const float CONTROLS_Y = PANEL_POSITION.y + PANEL_SIZE.y - FOOTER_HEIGHT + (FOOTER_HEIGHT - PLAY_SIZE.y) / 2.f;
    const float CONTROLS_X = PANEL_POSITION.x + PANEL_SIZE.x - 50.f - (SKIP_SIZE.x + 16.f + PLAY_SIZE.x + 16.f + SKIP_SIZE.x);

    // The sections, in the order the songs are listed; any other song goes to the section of its kind, an archived
    // version (a name ending in _v1, _v2...) to OLD VERSIONS. The battle start jingle's keyed copies are left out
    struct Section { const char* title; std::vector<const char*> names; };
    const std::vector<Section> SECTIONS = {
        { "MENUS", { "theme_title", "theme_base", "theme_map", "theme_lab", "theme_clam", "theme_clam_inside", "theme_shop" } },
        { "STAGES", { "theme_default", "theme_calm", "theme_serious", "theme_stressful", "theme_crab", "theme_shanty", "theme_jelly",
            "theme_danger", "theme_energetic", "theme_neon", "theme_preboss" } },
        { "BOSSES", { "theme_boss", "theme_crab_boss", "theme_jelly_boss", "theme_energetic_boss", "theme_neon_boss" } },
        { "JINGLES", { "jingle_battle_start", "jingle_victory", "jingle_defeat" } },
        { "OLD VERSIONS", {} },
    };

    std::string minutes(float seconds)
    {
        const int total = std::max(0, static_cast<int>(seconds));
        const int s = total % 60;
        return std::to_string(total / 60) + (s < 10 ? ":0" : ":") + std::to_string(s);
    }

    // An outlined triangle pointing right (or left), centered on a point
    void draw_triangle(sf::RenderWindow& window, sf::Vector2f center, float size, bool isLeft, sf::Color fill)
    {
        sf::ConvexShape shape(3);
        const float d = isLeft ? -1.f : 1.f;
        shape.setPoint(0, { center.x - d * size * 0.45f, center.y - size * 0.55f });
        shape.setPoint(1, { center.x + d * size * 0.55f, center.y });
        shape.setPoint(2, { center.x - d * size * 0.45f, center.y + size * 0.55f });
        shape.setFillColor(fill);
        shape.setOutlineThickness(3.f);
        shape.setOutlineColor(UISkin::OUTLINE);
        window.draw(shape);
    }

    void draw_box(sf::RenderWindow& window, sf::FloatRect rect, sf::Color fill)
    {
        sf::RectangleShape box(rect.size);
        box.setPosition(rect.position);
        box.setFillColor(fill);
        box.setOutlineThickness(3.f);
        box.setOutlineColor(UISkin::OUTLINE);
        window.draw(box);
    }
}

UIPlaylist::UIPlaylist(std::shared_ptr<AudioManager> audio) :
    UIElement(SCREEN_SIZE, { 0.f, 0.f }),
    m_audio(std::move(audio)),
    m_dim(SCREEN_SIZE),
    m_frame({ PANEL_POSITION, PANEL_SIZE }, "Playlist", FOOTER_HEIGHT),
    m_previousButton(SKIP_SIZE, { CONTROLS_X, CONTROLS_Y }, ""),
    m_playButton(PLAY_SIZE, { CONTROLS_X + SKIP_SIZE.x + 16.f, CONTROLS_Y }, ""),
    m_nextButton(SKIP_SIZE, { CONTROLS_X + SKIP_SIZE.x + 16.f + PLAY_SIZE.x + 16.f, CONTROLS_Y }, "")
{
    set_z(Z_WINDOW);
    m_dim.setFillColor(sf::Color(0, 0, 0, 120));
    m_frame.set_on_close([this]() { close(); });

    m_previousButton.set_style(UIButtonElement::Style::Secondary);
    m_nextButton.set_style(UIButtonElement::Style::Secondary);

    auto firstSong = [this]() { return next_song(-1, 1); };
    m_previousButton.set_callback([this, firstSong]() { play(next_song(m_selected >= 0 ? m_selected : firstSong(), -1), true); });
    m_nextButton.set_callback([this, firstSong]() { play(m_selected >= 0 ? next_song(m_selected, 1) : firstSong(), true); });
    m_playButton.set_callback([this, firstSong]()
        {
            if (m_playing >= 0) stop();
            else play(m_selected >= 0 ? m_selected : firstSong(), true);
        });

    m_isVisible = false;
}

// ===== The list =====

void UIPlaylist::build_rows()
{
    m_rows.clear();
    m_texts.clear();
    if (!m_audio) return;

    // Each song in its section
    std::map<std::string, AudioManager::Listing> songs;
    for (const auto& listing : m_audio->get_music_list())
        if (listing.name.rfind("jingle_battle_start_", 0) != 0) songs[listing.name] = listing;
    std::vector<std::vector<AudioManager::Listing>> bySection(SECTIONS.size());
    for (size_t s = 0; s < SECTIONS.size(); ++s)
        for (const char* name : SECTIONS[s].names)
            if (auto found = songs.find(name); found != songs.end()) { bySection[s].push_back(found->second); songs.erase(found); }
    const std::regex archived(".*_v[0-9]+$");
    std::vector<AudioManager::Listing> old;
    for (const auto& [name, listing] : songs)
    {
        if (std::regex_match(name, archived)) old.push_back(listing);
        else bySection[listing.kind == "menu" ? 0 : listing.kind == "jingle" ? 3 : name.find("boss") != std::string::npos ? 2 : 1].push_back(listing);
    }
    std::sort(old.begin(), old.end(), [](const auto& a, const auto& b) { return a.title < b.title; });
    bySection.back() = old;

    // The rows: a header, then its songs; two texts each (the title or the section's name, the length)
    float top = 0.f;
    auto addText = [this](const std::string& text, unsigned int size, sf::Color color, bool isBody)
        {
            sf::Text label(isBody ? UISkin::regular_font() : UISkin::heavy_font(), text);
            if (isBody) UISkin::style_body_text(label, size, color);
            else UISkin::style_text(label, size, color);
            m_texts.push_back(label);
        };
    for (size_t s = 0; s < SECTIONS.size(); ++s)
    {
        if (bySection[s].empty()) continue;
        m_rows.push_back({ "", SECTIONS[s].title, false, 0.f, top, HEADER_ROW });
        addText(SECTIONS[s].title, HEADER_SIZE, UISkin::TITLE, false);
        addText("", LENGTH_SIZE, LENGTH_COLOR, true);
        top += HEADER_ROW;
        for (const auto& song : bySection[s])
        {
            m_rows.push_back({ song.name, song.title, song.isJingle, song.seconds, top, SONG_ROW });
            addText(song.title, TITLE_SIZE, UISkin::TEXT, false);
            addText(song.seconds > 0.f ? minutes(song.seconds) : "", LENGTH_SIZE, LENGTH_COLOR, true);
            top += SONG_ROW;
        }
    }
    m_contentHeight = top;
}

sf::FloatRect UIPlaylist::list_rect() const
{
    const sf::FloatRect body = m_frame.get_body();
    return { { body.position.x + 40.f, body.position.y + 12.f }, { body.size.x - 100.f, body.size.y - 24.f } };
}

float UIPlaylist::hidden_height() const { return std::max(0.f, m_contentHeight - list_rect().size.y); }

int UIPlaylist::row_at(float listY) const
{
    const float y = listY + m_scroll;
    for (size_t i = 0; i < m_rows.size(); ++i)
        if (y >= m_rows[i].top && y < m_rows[i].top + m_rows[i].height) return static_cast<int>(i);
    return -1;
}

int UIPlaylist::row_of(const std::string& name) const
{
    if (name.empty()) return -1;
    for (size_t i = 0; i < m_rows.size(); ++i) if (m_rows[i].name == name) return static_cast<int>(i);
    return -1;
}

int UIPlaylist::next_song(int from, int step) const
{
    const int count = static_cast<int>(m_rows.size());
    if (count == 0) return -1;
    int row = from;
    for (int tries = 0; tries < count; ++tries)
    {
        row = ((row + step) % count + count) % count;
        if (!m_rows[row].name.empty()) return row;
    }
    return -1;
}

void UIPlaylist::scroll_to(int row)
{
    if (row < 0 || row >= static_cast<int>(m_rows.size())) return;
    const float visible = list_rect().size.y;
    const Row& target = m_rows[row];
    // Its section's header shows too when it can
    const float top = row > 0 && m_rows[row - 1].name.empty() ? m_rows[row - 1].top : target.top;
    if (top < m_scroll) m_scroll = top;
    else if (target.top + target.height > m_scroll + visible) m_scroll = target.top + target.height - visible;
    m_scroll = std::clamp(m_scroll, 0.f, hidden_height());
}

// ===== Playing =====

void UIPlaylist::play(int row, bool follow)
{
    if (!m_audio || row < 0 || row >= static_cast<int>(m_rows.size()) || m_rows[row].name.empty()) return;
    const Row& song = m_rows[row];
    if (song.isJingle) m_audio->play_jingle(song.name);
    else m_audio->play_music(song.name, m_playing >= 0 ? 1.2f : 0.6f);
    m_playing = m_selected = row;
    m_lastPosition = m_audio->get_music_position();
    if (follow) scroll_to(row);
}

void UIPlaylist::stop()
{
    if (m_audio) m_audio->stop_music(0.6f);
    m_playing = -1;
}

void UIPlaylist::open()
{
    if (m_rows.empty()) build_rows();
    if (m_audio)
    {
        m_previousMusic = m_audio->get_current_music();
        m_previousPosition = m_audio->get_music_position();
        m_wasDucked = m_audio->is_music_ducked();
        m_audio->set_music_ducked(false);   // The pause menu's quieter music: full while listening
    }
    m_playing = m_selected = row_of(m_previousMusic);
    m_lastPosition = m_previousPosition;
    m_isPressed = false;
    m_hovered = -1;
    m_scroll = 0.f;
    scroll_to(m_playing);
    m_isVisible = true;
}

void UIPlaylist::close()
{
    if (!m_isVisible) return;
    m_isVisible = false;
    if (!m_audio) return;

    // The music that was playing comes back where it was (a jingle doesn't: it was ending)
    if (m_audio->get_current_music() != m_previousMusic)
    {
        if (m_previousMusic.empty() || m_previousMusic.rfind("jingle", 0) == 0) m_audio->stop_music(0.6f);
        else m_audio->resume_music(m_previousMusic, m_previousPosition, 1.f);
    }
    m_audio->set_music_ducked(m_wasDucked);
}

void UIPlaylist::update(float deltaTime)
{
    if (!m_isVisible) return;
    m_clock += deltaTime;
    m_frame.update(deltaTime);
    m_previousButton.update(deltaTime);
    m_playButton.update(deltaTime);
    m_nextButton.update(deltaTime);

    // The song playing: a jingle that ended, or a theme back at its loop (it played through): the next one
    if (m_playing >= 0 && m_audio)
    {
        const Row& song = m_rows[m_playing];
        const std::string& current = m_audio->get_current_music();
        if (current != song.name)
        {
            if (current.empty()) play(next_song(m_playing, 1), true);
            else m_playing = row_of(current);
        }
        else if (!song.isJingle)
        {
            const float position = m_audio->get_music_position();
            if (position + 1.f < m_lastPosition) play(next_song(m_playing, 1), true);
            else m_lastPosition = position;
        }
    }
}

// ===== Drawing =====

void UIPlaylist::render(sf::RenderWindow& window)
{
    if (!m_isVisible) return;
    // Over the Options window: the whole screen (a phone's sides too, already dimmed once like the layout)
    const sf::FloatRect screen = UISkin::screen_rect();
    m_dim.setPosition(screen.position);
    m_dim.setSize(screen.size);
    window.draw(m_dim);
    m_frame.render(window);
    render_list(window);
    render_footer(window);
}

void UIPlaylist::render_list(sf::RenderWindow& window)
{
    const sf::FloatRect area = list_rect();

    // Clipped to its area (it scrolls inside)
    const sf::View view = window.getView();
    const sf::Vector2f windowSize(window.getSize());
    const sf::Vector2i topLeft = window.mapCoordsToPixel(area.position, view);
    const sf::Vector2i bottomRight = window.mapCoordsToPixel(area.position + area.size, view);
    sf::View clipped = view;
    const sf::FloatRect scissor{ { std::clamp(topLeft.x / windowSize.x, 0.f, 1.f), std::clamp(topLeft.y / windowSize.y, 0.f, 1.f) },
        { std::clamp((bottomRight.x - topLeft.x) / windowSize.x, 0.f, 1.f), std::clamp((bottomRight.y - topLeft.y) / windowSize.y, 0.f, 1.f) } };
    if (scissor.size.x > 0.f && scissor.size.y > 0.f) clipped.setScissor(scissor);
    window.setView(clipped);

    const float scroll = std::round(m_scroll);
    for (size_t i = 0; i < m_rows.size(); ++i)
    {
        const Row& row = m_rows[i];
        const float top = area.position.y + row.top - scroll;
        if (top + row.height < area.position.y || top > area.position.y + area.size.y) continue;
        const float middle = top + row.height / 2.f;
        sf::Text& title = m_texts[i * 2];
        sf::Text& length = m_texts[i * 2 + 1];

        if (row.name.empty())
        {
            // A section: its name and a line under it
            UISkin::set_text_anchor(title, { 0.f, 0.5f });
            title.setPosition({ area.position.x + 8.f, middle + 4.f });
            window.draw(title);
            sf::RectangleShape line({ area.size.x - 16.f, 3.f });
            line.setPosition({ area.position.x + 8.f, top + row.height - 10.f });
            line.setFillColor(sf::Color(UISkin::TITLE.r, UISkin::TITLE.g, UISkin::TITLE.b, 70));
            window.draw(line);
            continue;
        }

        const bool isPlaying = static_cast<int>(i) == m_playing;
        if (isPlaying || static_cast<int>(i) == m_hovered)
        {
            sf::RectangleShape band({ area.size.x, row.height - 6.f });
            band.setPosition({ area.position.x, top + 3.f });
            band.setFillColor(isPlaying ? sf::Color(255, 222, 48, 42) : sf::Color(255, 255, 255, 20));
            window.draw(band);
        }

        // The song playing: an equalizer; the others: a play triangle
        const sf::Vector2f iconCenter{ area.position.x + 34.f, middle };
        if (isPlaying)
        {
            for (int bar = 0; bar < 3; ++bar)
            {
                const float h = 8.f + 18.f * (0.5f + 0.5f * std::sin(m_clock * (7.f + 2.3f * bar) + bar * 1.7f));
                draw_box(window, { { iconCenter.x - 15.f + bar * 11.f, middle + 13.f - h }, { 7.f, h } }, UISkin::NUMBER);
            }
        }
        else draw_triangle(window, iconCenter, 22.f, false, sf::Color(255, 255, 255, 190));

        title.setFillColor(isPlaying ? UISkin::NUMBER : UISkin::TEXT);
        UISkin::set_text_anchor(title, { 0.f, 0.5f });
        title.setPosition({ area.position.x + 70.f, middle });
        window.draw(title);
        UISkin::set_text_anchor(length, { 1.f, 0.5f });
        length.setPosition({ area.position.x + area.size.x - 16.f, middle });
        window.draw(length);
    }
    window.setView(view);

    // A bar on the right shows where the visible part is
    const float hidden = hidden_height();
    if (hidden > 1.f)
    {
        const float trackX = area.position.x + area.size.x + 22.f;
        sf::RectangleShape track({ 10.f, area.size.y });
        track.setPosition({ trackX, area.position.y });
        track.setFillColor(sf::Color(255, 255, 255, 40));
        window.draw(track);
        const float thumbHeight = std::max(60.f, area.size.y * area.size.y / m_contentHeight);
        sf::RectangleShape thumb({ 10.f, thumbHeight });
        thumb.setPosition({ trackX, area.position.y + (area.size.y - thumbHeight) * (m_scroll / hidden) });
        thumb.setFillColor(sf::Color(255, 255, 255, 200));
        window.draw(thumb);
    }
}

void UIPlaylist::render_footer(sf::RenderWindow& window)
{
    const sf::FloatRect footer = m_frame.get_footer();
    const float left = footer.position.x + 50.f;
    const float barRight = m_previousButton.get_bounds().position.x - 60.f;

    // The song playing (or what to do), how far it is
    const Row* song = m_playing >= 0 ? &m_rows[m_playing] : m_selected >= 0 ? &m_rows[m_selected] : nullptr;
    sf::Text title(UISkin::heavy_font(), song ? song->title : "Tap a song to play it");
    UISkin::style_text(title, 32, m_playing >= 0 ? UISkin::NUMBER : UISkin::TEXT);
    UISkin::set_text_anchor(title, { 0.f, 0.5f });
    title.setPosition({ left, footer.position.y + 58.f });
    window.draw(title);

    const float position = m_playing >= 0 && m_audio ? m_audio->get_music_position() : 0.f;
    const float total = song ? song->seconds : 0.f;
    sf::Text time(UISkin::heavy_font(), minutes(position) + " / " + minutes(total));
    UISkin::style_text(time, 26, UISkin::TEXT);   // Outlined: the footer is light
    UISkin::set_text_anchor(time, { 1.f, 0.5f });
    time.setPosition({ barRight, footer.position.y + 122.f });
    window.draw(time);

    const float barWidth = std::max(100.f, barRight - left - time.getLocalBounds().size.x - 30.f);
    sf::RectangleShape bar({ barWidth, 14.f });
    bar.setPosition({ left, footer.position.y + 115.f });
    bar.setFillColor(sf::Color(0, 0, 0, 90));
    bar.setOutlineThickness(3.f);
    bar.setOutlineColor(UISkin::OUTLINE);
    window.draw(bar);
    if (total > 0.f && m_playing >= 0)
    {
        sf::RectangleShape fill({ barWidth * std::clamp(position / total, 0.f, 1.f), 14.f });
        fill.setPosition({ left, footer.position.y + 115.f });
        fill.setFillColor(UISkin::NUMBER);
        window.draw(fill);
    }

    // Previous, play or stop, next: drawn icons over the buttons
    m_previousButton.render(window);
    m_playButton.render(window);
    m_nextButton.render(window);
    auto centerOf = [](const UIButtonElement& button) { const sf::FloatRect b = button.get_bounds(); return b.position + b.size / 2.f - sf::Vector2f{ 0.f, 3.f }; };
    const sf::Vector2f previous = centerOf(m_previousButton), playing = centerOf(m_playButton), next = centerOf(m_nextButton);
    draw_box(window, { { previous.x - 22.f, previous.y - 15.f }, { 7.f, 30.f } }, UISkin::TEXT);
    draw_triangle(window, { previous.x + 5.f, previous.y }, 30.f, true, UISkin::TEXT);
    draw_box(window, { { next.x + 15.f, next.y - 15.f }, { 7.f, 30.f } }, UISkin::TEXT);
    draw_triangle(window, { next.x - 5.f, next.y }, 30.f, false, UISkin::TEXT);
    if (m_playing >= 0) draw_box(window, { { playing.x - 16.f, playing.y - 16.f }, { 32.f, 32.f } }, UISkin::TEXT);
    else draw_triangle(window, { playing.x + 3.f, playing.y }, 38.f, false, UISkin::TEXT);
}

// ===== Input =====

void UIPlaylist::handle_event(const sf::Event& event, const sf::RenderWindow& window)
{
    if (!m_isVisible) return;
    const sf::FloatRect area = list_rect();
    const float hidden = hidden_height();

    if (const auto* key = event.getIf<sf::Event::KeyPressed>())
    {
        if (key->code == sf::Keyboard::Key::Escape)
        {
            play_sound("ui_back");
            close();
            return;
        }
        if (key->code == sf::Keyboard::Key::Down) m_scroll = std::min(hidden, m_scroll + SONG_ROW * 2.f);
        else if (key->code == sf::Keyboard::Key::Up) m_scroll = std::max(0.f, m_scroll - SONG_ROW * 2.f);
    }
    else if (const auto* wheel = event.getIf<sf::Event::MouseWheelScrolled>())
        m_scroll = std::clamp(m_scroll - wheel->delta * SONG_ROW * 1.5f, 0.f, hidden);
    else if (const auto* pressed = event.getIf<sf::Event::MouseButtonPressed>(); pressed && pressed->button == sf::Mouse::Button::Left)
    {
        const sf::Vector2f point = window.mapPixelToCoords(pressed->position);
        if (area.contains(point))
        {
            m_isPressed = true;
            m_hasDragged = false;
            m_dragStartY = point.y;
            m_dragStartScroll = m_scroll;
            m_hovered = row_at(point.y - area.position.y);
        }
    }
    else if (const auto* moved = event.getIf<sf::Event::MouseMoved>())
    {
        const sf::Vector2f point = window.mapPixelToCoords(moved->position);
        if (m_isPressed)
        {
            const float dragY = point.y - m_dragStartY;
            if (std::abs(dragY) > DRAG_THRESHOLD) { m_hasDragged = true; m_hovered = -1; }
            if (m_hasDragged) m_scroll = std::clamp(m_dragStartScroll - dragY, 0.f, hidden);
        }
        else m_hovered = area.contains(point) ? row_at(point.y - area.position.y) : -1;
    }
    else if (const auto* released = event.getIf<sf::Event::MouseButtonReleased>(); released && released->button == sf::Mouse::Button::Left)
    {
        // A tap (not a slide) on a song plays it
        const sf::Vector2f point = window.mapPixelToCoords(released->position);
        if (m_isPressed && !m_hasDragged && area.contains(point))
        {
            const int row = row_at(point.y - area.position.y);
            if (row >= 0 && !m_rows[row].name.empty() && row != m_playing) play(row, false);
        }
        m_isPressed = false;
        if (m_hasDragged) m_hovered = -1;
    }

    m_previousButton.handle_event(event, window);
    m_playButton.handle_event(event, window);
    m_nextButton.handle_event(event, window);
    m_frame.handle_event(event, window);
}

sf::FloatRect UIPlaylist::get_bounds() const
{
    if (!m_isVisible) return sf::FloatRect{ { 0.f, 0.f }, { 0.f, 0.f } };
    return sf::FloatRect{ m_position, m_size };
}

bool UIPlaylist::is_blocking_input() const
{
    return m_isVisible;
}
