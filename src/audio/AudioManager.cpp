#include "AudioManager.h"

#include "../core/Platform.h"
#include "../external/nlohmann/json.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>

using json = nlohmann::json;

namespace
{
    const std::string MUSIC_LIST = "assets/audios/music/music.json";
    const std::string SOUND_LIST = "assets/audios/sfx/sfx.json";
    const std::string NO_MUSIC;
}

AudioManager::AudioManager() :
    m_random(std::random_device{}())
{
}

bool AudioManager::load()
{
    const bool musicLoaded = load_music_list(MUSIC_LIST);
    const bool soundsLoaded = load_sound_list(SOUND_LIST);
    return musicLoaded && soundsLoaded;
}

/*
 * music.json: { "theme_default": { "file": "...ogg", "sampleRate": 48000, "loopStart": 4092632, "loopLength": 3789474 }, ... }
 * Jingles have no loop.
 */
bool AudioManager::load_music_list(const std::string& path)
{
    auto file = Platform::open(path);
    if (!file)
    {
        std::cerr << "AudioManager: can't read " << path << "\n";
        return false;
    }

    try
    {
        const json list = json::parse(file);
        for (const auto& [name, value] : list.items())
        {
            MusicInfo info;
            info.file = value.at("file").get<std::string>();
            info.sampleRate = value.value("sampleRate", 48000u);
            info.loopStart = value.value("loopStart", std::uint64_t{ 0 });
            info.loopLength = value.value("loopLength", std::uint64_t{ 0 });
            info.length = value.value("length", 0.f);
            info.key = value.value("key", std::string{});
            m_musicInfos[name] = info;
        }
    }
    catch (const std::exception& error)
    {
        std::cerr << "AudioManager: " << path << ": " << error.what() << "\n";
        return false;
    }
    return true;
}

/*
 * sfx.json: { "ui_click": { "files": [...], "volume": 0.55, "pitch": 0.04, "cooldown": 0.03, "maxInstances": 2 }, ... }
 */
bool AudioManager::load_sound_list(const std::string& path)
{
    auto file = Platform::open(path);
    if (!file)
    {
        std::cerr << "AudioManager: can't read " << path << "\n";
        return false;
    }

    try
    {
        const json list = json::parse(file);
        for (const auto& [name, value] : list.items())
        {
            SoundInfo info;
            for (const auto& soundFile : value.at("files"))
            {
                auto buffer = std::make_unique<sf::SoundBuffer>();
                if (buffer->loadFromFile(soundFile.get<std::string>()))
                    info.buffers.push_back(std::move(buffer));
                else
                    std::cerr << "AudioManager: can't load " << soundFile.get<std::string>() << "\n";
            }
            info.volume = value.value("volume", 1.f);
            info.pitch = value.value("pitch", 0.f);
            info.cooldown = value.value("cooldown", 0.f);
            info.maxInstances = value.value("maxInstances", 4);
            info.priority = value.value("priority", 50);
            if (!info.buffers.empty()) m_sounds[name] = std::move(info);
        }
    }
    catch (const std::exception& error)
    {
        std::cerr << "AudioManager: " << path << ": " << error.what() << "\n";
        return false;
    }
    return true;
}

/*
 * Fades the tracks, drops the ones faded out and the sounds that finished
 */
void AudioManager::update(float realDeltaTime)
{
    m_clock += realDeltaTime;

    // Pause ducking ramps instead of jumping
    const float duckTarget = m_isDucked ? DUCKED_LEVEL : 1.f;
    const float duckStep = (1.f - DUCKED_LEVEL) * realDeltaTime / DUCK_RAMP_SECONDS;
    m_duckLevel = duckTarget < m_duckLevel ? std::max(duckTarget, m_duckLevel - duckStep) : std::min(duckTarget, m_duckLevel + duckStep);

    auto fade = [realDeltaTime](Track& track)
        {
            if (track.fadeSpeed <= 0.f) return;
            const float step = track.fadeSpeed * realDeltaTime;
            if (track.level < track.target) track.level = std::min(track.target, track.level + step);
            else track.level = std::max(track.target, track.level - step);
            if (track.level == track.target) track.fadeSpeed = 0.f;
        };

    if (m_current)
    {
        fade(*m_current);
        apply_volume(*m_current);

        // A queued theme starts once the jingle's last note ends; the jingle's reverb rings out under it
        const bool isStopped = m_current->music->getStatus() == sf::SoundSource::Status::Stopped;
        if (m_current->isJingle && !m_queuedMusic.empty()
            && (isStopped || (m_current->length > 0.f && m_current->music->getPlayingOffset().asSeconds() >= m_current->length)))
        {
            const std::string next = m_queuedMusic;
            m_current->target = 0.f;
            m_current->fadeSpeed = 1.f / 0.35f;   // Only the crash and the room ring under the theme's first beat
            m_fadingOut.push_back(std::move(m_current));
            play_music(next, 0.f);
        }
        // A jingle that ended is done
        else if (isStopped && m_current->fadeSpeed == 0.f)
            m_current.reset();
    }

    for (auto& track : m_fadingOut)
    {
        fade(*track);
        apply_volume(*track);
    }
    std::erase_if(m_fadingOut, [](const std::unique_ptr<Track>& track)
        {
            return track->level <= 0.f || track->music->getStatus() == sf::SoundSource::Status::Stopped;
        });

    m_voices.remove_if([](const Voice& voice) { return voice.sound->getStatus() == sf::SoundSource::Status::Stopped; });

    // Focus fade: everything (music and sounds) through the listener's global volume
    const float focusTarget = (m_isUnfocused && m_muteInBackground) ? 0.f : 1.f;
    if (m_focusLevel != focusTarget)
    {
        const float step = realDeltaTime / FOCUS_FADE_SECONDS;
        m_focusLevel = focusTarget > m_focusLevel ? std::min(focusTarget, m_focusLevel + step) : std::max(focusTarget, m_focusLevel - step);
        sf::Listener::setGlobalVolume(m_masterVolume * m_focusLevel);
    }
}

// ==================
// Music
// ==================

std::unique_ptr<AudioManager::Track> AudioManager::open_track(const std::string& name)
{
    const auto info = m_musicInfos.find(name);
    if (info == m_musicInfos.end())
    {
        if (std::find(m_reportedMissing.begin(), m_reportedMissing.end(), name) == m_reportedMissing.end())
        {
            std::cerr << "AudioManager: no music named \"" << name << "\" in " << MUSIC_LIST << "\n";
            m_reportedMissing.push_back(name);
        }
        return nullptr;
    }

    auto track = std::make_unique<Track>();
    track->name = name;
    track->music = std::make_unique<sf::Music>();
    if (!track->music->openFromFile(info->second.file))
    {
        std::cerr << "AudioManager: can't open " << info->second.file << "\n";
        return nullptr;
    }

    if (info->second.loopLength > 0)
    {
        // Sample-exact: the loop points were measured on the sample grid the music was written on
        const auto toTime = [rate = info->second.sampleRate](std::uint64_t samples)
            {
                return sf::microseconds(static_cast<std::int64_t>((samples * 1'000'000 + rate / 2) / rate));
            };
        track->music->setLoopPoints({ toTime(info->second.loopStart), toTime(info->second.loopLength) });
        track->music->setLooping(true);
    }
    else
    {
        track->isJingle = true;
        track->length = info->second.length;
    }
    return track;
}

void AudioManager::fade_out_current(float fadeSeconds)
{
    if (!m_current) return;

    if (fadeSeconds <= 0.f)
    {
        m_current->music->stop();
        m_current.reset();
        return;
    }
    m_current->target = 0.f;
    m_current->fadeSpeed = std::max(m_current->level, 0.01f) / fadeSeconds;
    m_fadingOut.push_back(std::move(m_current));
}

void AudioManager::play_music(const std::string& name, float fadeSeconds)
{
    m_queuedMusic.clear();
    if (m_current && m_current->name == name) return;

    auto track = open_track(name);
    if (!track) return;

    // The old theme fades out while the new one fades in
    fade_out_current(fadeSeconds);
    track->level = fadeSeconds > 0.f ? 0.f : 1.f;
    track->target = 1.f;
    track->fadeSpeed = fadeSeconds > 0.f ? 1.f / fadeSeconds : 0.f;
    apply_volume(*track);
    track->music->play();
    m_current = std::move(track);
}

void AudioManager::play_music_in_sync(const std::string& name, float fadeSeconds)
{
    m_queuedMusic.clear();
    if (m_current && m_current->name == name) return;
    const bool canSync = m_current && !m_current->isJingle && m_current->music->getStatus() == sf::SoundSource::Status::Playing;
    const sf::Time offset = canSync ? m_current->music->getPlayingOffset() : sf::Time::Zero;

    auto track = open_track(name);
    if (!track) return;

    fade_out_current(fadeSeconds);
    track->level = 0.f;
    track->target = 1.f;
    track->fadeSpeed = 1.f / std::max(0.01f, fadeSeconds);
    apply_volume(*track);
    track->music->play();
    if (canSync) track->music->setPlayingOffset(offset);   // Only works once playing
    m_current = std::move(track);
}

void AudioManager::play_jingle(const std::string& name)
{
    m_queuedMusic.clear();
    auto track = open_track(name);
    if (!track) return;

    fade_out_current(0.15f);
    track->level = 1.f;
    track->target = 1.f;
    apply_volume(*track);
    track->music->play();
    m_current = std::move(track);
}

void AudioManager::queue_music(const std::string& name)
{
    const bool isJinglePlaying = m_current && m_current->isJingle
        && m_current->music->getStatus() == sf::SoundSource::Status::Playing;
    if (isJinglePlaying) m_queuedMusic = name;
    else play_music(name);
}

void AudioManager::stop_music(float fadeSeconds)
{
    fade_out_current(fadeSeconds);
}

void AudioManager::set_music_ducked(bool isDucked)
{
    m_isDucked = isDucked;   // update() ramps the level
}

void AudioManager::set_battle_mix(bool isBattle)
{
    m_isBattleMix = isBattle;
    if (m_current) apply_volume(*m_current);
}

const std::string& AudioManager::get_current_music() const
{
    return m_current ? m_current->name : NO_MUSIC;
}

bool AudioManager::has_music(const std::string& name) const
{
    return m_musicInfos.contains(name);
}

const std::string& AudioManager::get_music_key(const std::string& name) const
{
    static const std::string none;
    const auto found = m_musicInfos.find(name);
    return found != m_musicInfos.end() ? found->second.key : none;
}

float AudioManager::get_music_length(const std::string& name) const
{
    const auto found = m_musicInfos.find(name);
    return found != m_musicInfos.end() ? found->second.length : 0.f;
}

int AudioManager::key_distance(const std::string& from, const std::string& to)
{
    auto pitchClass = [](const std::string& key) -> int
        {
            if (key.empty()) return -1;
            static const int letters[] = { 9, 11, 0, 2, 4, 5, 7 };   // A B C D E F G
            if (key[0] < 'A' || key[0] > 'G') return -1;
            int pc = letters[key[0] - 'A'];
            if (key.size() > 1) pc += key[1] == '#' ? 1 : key[1] == 'b' ? -1 : 0;
            return (pc + 12) % 12;
        };
    const int a = pitchClass(from), b = pitchClass(to);
    if (a < 0 || b < 0) return 0;
    int distance = (b - a + 12) % 12;
    if (distance > 5) distance -= 12;
    return distance;
}

void AudioManager::apply_volume(Track& track) const
{
    const float battle = m_isBattleMix ? BATTLE_MUSIC_LEVEL : 1.f;
    track.music->setVolume(std::clamp(m_musicVolume * track.level * m_duckLevel * battle, 0.f, 100.f));
}

// ==================
// Sounds
// ==================

void AudioManager::play_sound(const std::string& name, float volumeScale, float pan, float pitch)
{
    const auto found = m_sounds.find(name);
    if (found == m_sounds.end())
    {
        if (!name.empty() && std::find(m_reportedMissing.begin(), m_reportedMissing.end(), name) == m_reportedMissing.end())
        {
            std::cerr << "AudioManager: no sound named \"" << name << "\" in " << SOUND_LIST << "\n";
            m_reportedMissing.push_back(name);
        }
        return;
    }
    SoundInfo& info = found->second;

    // Too soon after the last one: skipped
    if (m_clock - info.lastPlayed < info.cooldown) return;

    // Too many of it at once: the oldest one gives its place
    const auto playing = std::count_if(m_voices.begin(), m_voices.end(), [&name](const Voice& voice) { return voice.name == name; });
    if (playing >= info.maxInstances)
    {
        auto oldest = std::find_if(m_voices.begin(), m_voices.end(), [&name](const Voice& voice) { return voice.name == name; });
        if (oldest != m_voices.end()) m_voices.erase(oldest);
    }

    // All the voices are busy (the last few are kept for the important sounds): the oldest voice of
    // a lower or equal priority gives its place, else this one is skipped
    const size_t limit = info.priority >= 80 ? MAX_VOICES : MAX_VOICES - RESERVED_VOICES;
    if (m_voices.size() >= limit)
    {
        auto victim = m_voices.end();
        for (auto it = m_voices.begin(); it != m_voices.end(); ++it)
            if (it->priority <= info.priority && (victim == m_voices.end() || it->priority < victim->priority
                || (it->priority == victim->priority && it->started < victim->started)))
                victim = it;
        if (victim == m_voices.end()) return;
        m_voices.erase(victim);
    }

    std::uniform_int_distribution<size_t> pickVariant(0, info.buffers.size() - 1);
    std::uniform_real_distribution<float> pickPitch(-info.pitch, info.pitch);

    Voice voice{ name, std::make_unique<sf::Sound>(*info.buffers[pickVariant(m_random)]), info.priority, m_clock };
    voice.sound->setPan(std::clamp(pan, -1.f, 1.f));
    voice.sound->setVolume(std::clamp(m_soundVolume * info.volume * volumeScale, 0.f, 100.f));
    voice.sound->setPitch(pitch * (1.f + (info.pitch > 0.f ? pickPitch(m_random) : 0.f)));
    voice.sound->play();
    m_voices.push_back(std::move(voice));
    info.lastPlayed = m_clock;
}

bool AudioManager::has_sound(const std::string& name) const
{
    return m_sounds.contains(name);
}

// ==================
// Volumes
// ==================

void AudioManager::set_music_volume(float volume)
{
    m_musicVolume = std::clamp(volume, 0.f, 100.f);
    if (m_current) apply_volume(*m_current);
}

void AudioManager::set_sound_volume(float volume)
{
    m_soundVolume = std::clamp(volume, 0.f, 100.f);
}

void AudioManager::apply_settings(const Settings& settings)
{
    auto curve = [](float slider) { slider = std::clamp(slider, 0.f, 1.f); return slider * slider; };

    m_masterVolume = 100.f * curve(settings.masterVolume);
    sf::Listener::setGlobalVolume(m_masterVolume * m_focusLevel);
    set_music_volume(MUSIC_REFERENCE * curve(settings.musicVolume));
    set_sound_volume(100.f * curve(settings.effectsVolume));
    m_muteInBackground = settings.muteInBackground;
}

void AudioManager::set_focus_muted(bool isUnfocused)
{
    m_isUnfocused = isUnfocused;
}
