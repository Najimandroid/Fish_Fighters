#pragma once

#include <SFML/Audio.hpp>

#include <cstdint>
#include <list>
#include <map>
#include <memory>
#include <random>
#include <string>
#include <vector>

/*
 * AudioManager
 * ------------
 * Plays the game's music and sound effects.
 *
 * Responsibilities:
 *  - Loads assets/audios/music/music.json (the themes and their loop points in samples) and
 *    assets/audios/sfx/sfx.json (every sound: its files, volume, pitch variation, cooldown and instance limit)
 *  - Music: streams one theme at a time and loops it sample-exactly (the intro once, then the loop forever);
 *    a new theme crossfades with the old one; jingles play once; the music gets quieter while the game is paused
 *  - Sounds: a random variant, a slightly random pitch, and a sound played again too soon or too many times at once
 *    is skipped, so a crowd of fish attacking together stays readable
 *
 * The music, the jingles and the sounds are made by the scripts of tools/music (see compose_music.js and make_sfx.js).
 *
 * Example usage:
 *   AudioManager audio;
 *   audio.load();
 *   audio.play_music("theme_title");
 *   audio.play_sound("ui_click");
 *   audio.update(realDeltaTime);   // every frame
 */
class AudioManager
{
public:
    AudioManager();

    // Reads both lists and loads the sounds. Returns false if one can't be read (the game then plays without it)
    bool load();

    // Fades and finished sounds. Real time: the music keeps fading while the battle is paused
    void update(float realDeltaTime);

    // ===== Music =====
    // Crossfades to this theme (nothing if it's already playing; an unknown name is reported once and ignored)
    void play_music(const std::string& name, float fadeSeconds = 0.8f);

    // The music fades out quickly and the jingle plays once
    void play_jingle(const std::string& name);

    // Plays this theme once the jingle playing ends (the battle start jingle, then the stage's theme);
    // without a jingle it plays right away. play_music and play_jingle cancel it
    void queue_music(const std::string& name);

    void stop_music(float fadeSeconds = 0.6f);

    // The pause menu: the music plays quieter while it's open
    void set_music_ducked(bool isDucked);

    const std::string& get_current_music() const;
    bool has_music(const std::string& name) const;

    // ===== Sounds =====
    // volumeScale multiplies the sound's own volume (sfx.json)
    void play_sound(const std::string& name, float volumeScale = 1.f);
    bool has_sound(const std::string& name) const;

    // ===== Volumes (0 to 100) =====
    void set_music_volume(float volume);
    void set_sound_volume(float volume);

private:
    struct MusicInfo
    {
        std::string file;
        unsigned int sampleRate = 48000;
        std::uint64_t loopStart = 0;      // In samples (per channel): where the loop starts
        std::uint64_t loopLength = 0;     // 0: plays once (jingles)
        float length = 0.f;               // Jingles: seconds until their last note ends (a queued theme starts there)
    };

    struct SoundInfo
    {
        std::vector<std::unique_ptr<sf::SoundBuffer>> buffers;   // Variants (one is picked at random)
        float volume = 1.f;          // 0 to 1
        float pitch = 0.f;           // Random +- variation of the pitch (0.05 = 5%)
        float cooldown = 0.f;        // Seconds before it can play again
        int maxInstances = 4;        // At most this many at once
        float lastPlayed = -1000.f;  // Clock of the last time it played
    };

    // A theme or a jingle being played, with its own fade
    struct Track
    {
        std::string name;
        std::unique_ptr<sf::Music> music;
        bool isJingle = false;       // Plays once (no loop points)
        float length = 0.f;          // Jingles: where the last note ends
        float level = 0.f;           // 0 to 1, before the music volume and the ducking
        float target = 1.f;          // Level it fades to
        float fadeSpeed = 0.f;       // Level per second (0: no fade)
    };

    struct Voice
    {
        std::string name;
        std::unique_ptr<sf::Sound> sound;
    };

    bool load_music_list(const std::string& path);
    bool load_sound_list(const std::string& path);

    // Opens a theme or a jingle; nullptr if it can't
    std::unique_ptr<Track> open_track(const std::string& name);

    void fade_out_current(float fadeSeconds);
    void apply_volume(Track& track) const;

    std::map<std::string, MusicInfo> m_musicInfos;
    std::map<std::string, SoundInfo> m_sounds;

    std::unique_ptr<Track> m_current;                  // The theme (or jingle) playing
    std::vector<std::unique_ptr<Track>> m_fadingOut;   // Previous ones, fading out
    std::list<Voice> m_voices;                         // Sounds playing

    float m_clock = 0.f;               // Seconds since the start (cooldowns)
    float m_musicVolume = 70.f;
    float m_soundVolume = 100.f;
    bool m_isDucked = false;
    std::string m_queuedMusic;                         // Starts when the jingle playing ends
    std::vector<std::string> m_reportedMissing;        // Unknown names already reported (once each)

    std::mt19937 m_random;

    static constexpr float DUCKED_LEVEL = 0.4f;        // Music level while the game is paused
    static constexpr size_t MAX_VOICES = 48;           // Sounds at once, all together
};
