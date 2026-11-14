#pragma once

#include "GameBuilds.h"

#include <atomic>
#include <mutex>
#include <string>
#include <thread>

/*
 * GameBuilder
 * -----------
 * Rebuilds the game from the editor: runs "cmake --build <build folder> --target Fish_Fighters" in the
 * background with the Visual Studio compiler that configured that folder (both read from its CMakeCache.txt),
 * and keeps the output for the editor to show. Only the game is built: the editor running from the same
 * folder can't replace its own executable.
 * The build copies game_data (with player.json) and assets next to the game, like building in Visual Studio.
 */

class GameBuilder
{
public:

    enum class State { IDLE, RUNNING, SUCCEEDED, FAILED };

    GameBuilder() = default;
    ~GameBuilder();

    GameBuilder(const GameBuilder&) = delete;
    GameBuilder& operator=(const GameBuilder&) = delete;

    // Starts building (false if a build is already running or the folder can't be built)
    bool start(const GameBuilds::Build& build, std::string& error);

    // The phone version: builds the Android game, packages it and installs it on the phone plugged in over USB, then
    // starts it (node tools/android/android.js all [release]; Node.js and the Android tools needed)
    bool start_phone_install(const std::filesystem::path& projectFolder, bool isRelease, std::string& error);

    State get_state() const;
    const GameBuilds::Build& get_build() const;

    // Everything the build printed so far
    std::string get_log() const;

private:

    void run(std::string command);
    void append(const std::string& text);

    GameBuilds::Build m_build;
    std::thread m_thread;
    std::atomic<State> m_state{ State::IDLE };
    mutable std::mutex m_logMutex;
    std::string m_log;
};
