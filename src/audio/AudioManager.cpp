#include "AudioManager.h"

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
    std::ifstream file(path);
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
    std::ifstream file(path);
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
            m_current->fadeSpeed = 1.f / 1.5f;
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
    m_isDucked = isDucked;
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

void AudioManager::apply_volume(Track& track) const
{
    const float ducking = m_isDucked ? DUCKED_LEVEL : 1.f;
    track.music->setVolume(std::clamp(m_musicVolume * track.level * ducking, 0.f, 100.f));
}

// ==================
// Sounds
// ==================

void AudioManager::play_sound(const std::string& name, float volumeScale)
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

    // Too soon after the last one, or too many of them at once: skipped
    if (m_clock - info.lastPlayed < info.cooldown) return;
    const auto playing = std::count_if(m_voices.begin(), m_voices.end(), [&name](const Voice& voice) { return voice.name == name; });
    if (playing >= info.maxInstances) return;
    if (m_voices.size() >= MAX_VOICES) return;

    std::uniform_int_distribution<size_t> pickVariant(0, info.buffers.size() - 1);
    std::uniform_real_distribution<float> pickPitch(-info.pitch, info.pitch);

    Voice voice{ name, std::make_unique<sf::Sound>(*info.buffers[pickVariant(m_random)]) };
    voice.sound->setVolume(std::clamp(m_soundVolume * info.volume * volumeScale, 0.f, 100.f));
    voice.sound->setPitch(1.f + (info.pitch > 0.f ? pickPitch(m_random) : 0.f));
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
