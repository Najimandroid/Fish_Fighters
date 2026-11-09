#pragma once

#include <SFML/Audio.hpp>

#include <memory>
#include <string>
#include <vector>

class ProjectData;

/*
 * AudioLibrary
 * ------------
 * The game's music and sounds as the editor sees them, and a player to preview them.
 *
 * Responsibilities:
 *  - Reads the project's assets/audios/music/music.json (themes and jingles, with their titles and kinds)
 *    and assets/audios/sfx/sfx.json (sounds), the lists the game's AudioManager plays from
 *  - Plays one preview at a time (a theme from its start, or a sound); a new preview stops the previous one
 *
 * The music and sounds themselves are made by the scripts of tools/music (compose_music.js, make_sfx.js).
 */
class AudioLibrary
{
public:
    struct Entry
    {
        std::string name;     // Key in the list ("theme_boss", "attack_bite")
        std::string title;    // Music: its title ("Leviathan"); sounds: the name
        std::string kind;     // Music: "menu", "stage" or "jingle"; sounds: empty
        std::string file;     // "assets/..." path (sounds: their first variant)
    };

    explicit AudioLibrary(const ProjectData& project);

    // Reads both lists again (after a project reload)
    void refresh();

    const std::vector<Entry>& get_music() const;
    const std::vector<Entry>& get_sounds() const;
    const Entry* find_music(const std::string& name) const;
    const Entry* find_sound(const std::string& name) const;

    // ===== Preview =====
    void play(const Entry& entry, bool isMusic);
    void stop();
    bool is_playing(const std::string& name) const;   // The preview of this entry is playing

private:
    const ProjectData& m_project;
    std::vector<Entry> m_music;
    std::vector<Entry> m_sounds;

    std::unique_ptr<sf::Music> m_previewMusic;
    std::unique_ptr<sf::SoundBuffer> m_previewBuffer;
    std::unique_ptr<sf::Sound> m_previewSound;
    std::string m_playingName;
};
