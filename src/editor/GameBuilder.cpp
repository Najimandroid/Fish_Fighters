#include "GameBuilder.h"

#include <cstdio>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

namespace
{
    const char* GAME_TARGET = "Fish_Fighters";

    // "KEY:TYPE=value" line of CMakeCache.txt, empty if missing
    std::string cache_value(const fs::path& buildFolder, const std::string& key)
    {
        std::ifstream cache(buildFolder / "CMakeCache.txt");
        std::string line;
        while (std::getline(cache, line))
        {
            if (line.rfind(key + ":", 0) != 0) continue;
            const size_t equals = line.find('=');
            if (equals != std::string::npos) return line.substr(equals + 1);
        }
        return "";
    }

    // vcvars64.bat of the Visual Studio whose compiler configured the folder (".../VC/Tools/MSVC/.../cl.exe")
    fs::path vcvars_of(const std::string& compiler)
    {
        const size_t tools = compiler.find("/VC/Tools/");
        if (tools == std::string::npos) return {};
        return fs::path(compiler.substr(0, tools)) / "VC" / "Auxiliary" / "Build" / "vcvars64.bat";
    }
}

GameBuilder::~GameBuilder()
{
    // The build keeps running in its own process; only wait for its output to end
    if (m_thread.joinable()) m_thread.join();
}

bool GameBuilder::start_phone_install(const std::filesystem::path& projectFolder, bool isRelease, std::string& error)
{
    if (m_state == State::RUNNING) { error = "A build is already running"; return false; }
    if (m_thread.joinable()) m_thread.join();

    const fs::path tool = projectFolder / "tools" / "android" / "android.js";
    if (!fs::exists(tool)) { error = "No " + tool.string(); return false; }

    m_build = GameBuilds::Build{};
    m_build.name = isRelease ? "Android Release (phone)" : "Android Debug (phone)";
    {
        std::lock_guard<std::mutex> lock(m_logMutex);
        m_log = "Building " + m_build.name + ", then installing it on the phone (on a Xiaomi phone, tap Install on its prompt)...\n";
    }
    m_state = State::RUNNING;
    // cmd /c ""node" "tool" all [release] 2>&1"
    m_thread = std::thread(&GameBuilder::run, this, "\"node \"" + fs::path(tool).make_preferred().string() + "\" all" + (isRelease ? " release" : "") + " 2>&1\"");
    return true;
}

bool GameBuilder::start(const GameBuilds::Build& build, std::string& error)
{
    if (m_state == State::RUNNING) { error = "A build is already running"; return false; }
    if (m_thread.joinable()) m_thread.join();

    const std::string cmake = cache_value(build.folder, "CMAKE_COMMAND");
    const fs::path vcvars = vcvars_of(cache_value(build.folder, "CMAKE_CXX_COMPILER"));
    if (cmake.empty()) { error = "No CMakeCache.txt in " + build.folder.string() + ": open the project in Visual Studio once"; return false; }
    if (vcvars.empty() || !fs::exists(vcvars)) { error = "Visual Studio's vcvars64.bat wasn't found (" + vcvars.string() + ")"; return false; }

    // A small script: the compiler environment, then the build of the game only
    const fs::path script = fs::temp_directory_path() / "fish_editor_build.bat";
    {
        std::ofstream file(script, std::ios::trunc);
        file << "@echo off\n"
             << "call \"" << vcvars.string() << "\" >nul 2>&1 || exit /b 1\n"
             << "\"" << fs::path(cmake).make_preferred().string() << "\" --build \"" << build.folder.string() << "\" --target " << GAME_TARGET << "\n";
        if (!file) { error = "Can't write " + script.string(); return false; }
    }

    m_build = build;
    {
        std::lock_guard<std::mutex> lock(m_logMutex);
        m_log = "Building " + build.name + "...\n";
    }
    m_state = State::RUNNING;

    // cmd /c ""script" 2>&1": the outer quotes keep the path's own quotes
    m_thread = std::thread(&GameBuilder::run, this, "\"\"" + script.string() + "\" 2>&1\"");
    return true;
}

void GameBuilder::run(std::string command)
{
    FILE* pipe = _popen(command.c_str(), "r");
    if (!pipe)
    {
        append("Can't start the build\n");
        m_state = State::FAILED;
        return;
    }

    char buffer[512];
    while (std::fgets(buffer, sizeof(buffer), pipe)) append(buffer);

    const int exitCode = _pclose(pipe);
    append(exitCode == 0 ? "\nBuild succeeded.\n" : "\nBuild failed (exit code " + std::to_string(exitCode) + ").\n");
    m_state = exitCode == 0 ? State::SUCCEEDED : State::FAILED;
}

void GameBuilder::append(const std::string& text)
{
    std::lock_guard<std::mutex> lock(m_logMutex);
    m_log += text;
}

GameBuilder::State GameBuilder::get_state() const
{
    return m_state;
}

const GameBuilds::Build& GameBuilder::get_build() const
{
    return m_build;
}

std::string GameBuilder::get_log() const
{
    std::lock_guard<std::mutex> lock(m_logMutex);
    return m_log;
}
