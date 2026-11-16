#pragma once

#include "UIPopupFrame.h"
#include "UIButtonElement.h"

#include <memory>
#include <string>
#include <vector>

class AudioManager;

/*
 * UIPlaylist
 * ----------
 * The music room, opened from Options: every song of the game to listen to, in sections (MENUS, STAGES, BOSSES,
 * JINGLES, OLD VERSIONS: the archived versions of reworked songs). A tap on a song plays it; a theme plays through
 * (its intro and its loop twice, like a soundtrack album) and the next song follows. The bar at the bottom shows the
 * song playing, how far it is, and the previous / play or stop / next buttons. The list scrolls (a finger, the wheel,
 * the arrow keys) and follows the song playing. Closing it puts back the music that was playing, where it was, and
 * the pause menu's quieter music if it was opened from there.
 */
class UIPlaylist : public UIElement
{
public:
    explicit UIPlaylist(std::shared_ptr<AudioManager> audio);

    void open();
    void close();
    bool is_open() const { return m_isVisible; }

    void update(float deltaTime) override;
    void render(sf::RenderWindow& window) override;
    void handle_event(const sf::Event& event, const sf::RenderWindow& window) override;

    sf::FloatRect get_bounds() const override;
    bool is_blocking_input() const override;

private:
    struct Row
    {
        std::string name;           // The music's name ("" for a section header)
        std::string title;
        bool isJingle = false;
        float seconds = 0.f;
        float top = 0.f;            // In the list, before scrolling
        float height = 0.f;
    };

    void build_rows();
    void play(int row, bool follow);   // follow: scroll the list to it
    void stop();
    int next_song(int from, int step) const;   // The next song row from a row, either way (wraps around)
    int row_at(float listY) const;
    int row_of(const std::string& name) const;
    sf::FloatRect list_rect() const;
    float hidden_height() const;
    void scroll_to(int row);
    void render_list(sf::RenderWindow& window);
    void render_footer(sf::RenderWindow& window);

    std::shared_ptr<AudioManager> m_audio;
    sf::RectangleShape m_dim;
    UIPopupFrame m_frame;
    UIButtonElement m_previousButton;
    UIButtonElement m_playButton;
    UIButtonElement m_nextButton;

    std::vector<Row> m_rows;
    std::vector<sf::Text> m_texts;      // Per row: its title (or the section's name), then its length
    float m_contentHeight = 0.f;
    int m_playing = -1;                 // The row playing (-1: none)
    int m_selected = -1;                // The last song played (PLAY starts it again)
    int m_hovered = -1;
    float m_lastPosition = 0.f;         // The song's last position: when it goes back to the loop, it has played through
    float m_clock = 0.f;                // The equalizer's animation

    float m_scroll = 0.f;
    bool m_isPressed = false;           // A finger / the mouse down on the list
    bool m_hasDragged = false;
    float m_dragStartY = 0.f, m_dragStartScroll = 0.f;

    // The music before the playlist opened, put back when it closes
    std::string m_previousMusic;
    float m_previousPosition = 0.f;
    bool m_wasDucked = false;
};
