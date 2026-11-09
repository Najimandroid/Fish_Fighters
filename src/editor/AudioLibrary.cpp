#include "AudioLibrary.h"

#include "ProjectData.h"

#include <fstream>
#include <iostream>

namespace
{
    const char* MUSIC_LIST = "assets/audios/music/music.json";
    const char* SOUND_LIST = "assets/audios/sfx/sfx.json";
}

AudioLibrary::AudioLibrary(const ProjectData& project) :
    m_project(project)
{
    refresh();
}

void AudioLibrary::refresh()
{
    stop();
    m_music.clear();
    m_sounds.clear();

    auto read = [this](const char* path) -> nlohmann::json
        {
            std::ifstream file(m_project.resolve(path));
            if (!file) return nlohmann::json::object();
            try { return nlohmann::json::parse(file); }
            catch (const std::exception& error)
            {
                std::cerr << "AudioLibrary: " << path << ": " << error.what() << "\n";
                return nlohmann::json::object();
            }
        };

    for (const auto& [name, value] : read(MUSIC_LIST).items())
    {
        if (!value.is_object()) continue;
        m_music.push_back({ name, value.value("title", name), value.value("kind", "stage"), value.value("file", "") });
    }
    for (const auto& [name, value] : read(SOUND_LIST).items())
    {
        if (!value.is_object()) continue;
        std::string file;
        if (value.contains("files") && value["files"].is_array() && !value["files"].empty() && value["files"][0].is_string())
            file = value["files"][0].get<std::string>();
        m_sounds.push_back({ name, name, "", file });
    }
}

const std::vector<AudioLibrary::Entry>& AudioLibrary::get_music() const
{
    return m_music;
}

const std::vector<AudioLibrary::Entry>& AudioLibrary::get_sounds() const
{
    return m_sounds;
}

const AudioLibrary::Entry* AudioLibrary::find_music(const std::string& name) const
{
    for (const auto& entry : m_music) if (entry.name == name) return &entry;
    return nullptr;
}

const AudioLibrary::Entry* AudioLibrary::find_sound(const std::string& name) const
{
    for (const auto& entry : m_sounds) if (entry.name == name) return &entry;
    return nullptr;
}

void AudioLibrary::play(const Entry& entry, bool isMusic)
{
    stop();
    const std::string path = m_project.resolve(entry.file).string();

    if (isMusic)
    {
        m_previewMusic = std::make_unique<sf::Music>();
        if (!m_previewMusic->openFromFile(path)) { m_previewMusic.reset(); return; }
        m_previewMusic->play();
    }
    else
    {
        m_previewBuffer = std::make_unique<sf::SoundBuffer>();
        if (!m_previewBuffer->loadFromFile(path)) { m_previewBuffer.reset(); return; }
        m_previewSound = std::make_unique<sf::Sound>(*m_previewBuffer);
        m_previewSound->play();
    }
    m_playingName = entry.name;
}

void AudioLibrary::stop()
{
    if (m_previewMusic) m_previewMusic->stop();
    if (m_previewSound) m_previewSound->stop();
    m_previewMusic.reset();
    m_previewSound.reset();   // Before its buffer
    m_previewBuffer.reset();
    m_playingName.clear();
}

bool AudioLibrary::is_playing(const std::string& name) const
{
    if (m_playingName.empty() || m_playingName != name) return false;
    if (m_previewMusic) return m_previewMusic->getStatus() == sf::SoundSource::Status::Playing;
    if (m_previewSound) return m_previewSound->getStatus() == sf::SoundSource::Status::Playing;
    return false;
}
