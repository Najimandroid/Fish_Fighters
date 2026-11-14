#include "GameBuilds.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <shellapi.h>

#include <algorithm>

namespace fs = std::filesystem;

namespace
{
    const char* GAME_EXECUTABLE = "Fish_Fighters.exe";
}

namespace GameBuilds
{
    std::vector<Build> find(const fs::path& projectFolder)
    {
        std::vector<Build> builds;
        std::error_code error;
        for (const auto& entry : fs::directory_iterator(projectFolder / "out" / "build", error))
        {
            if (entry.is_directory() && fs::exists(entry.path() / GAME_EXECUTABLE))
                if (entry.path().filename().string().rfind("android-", 0) != 0)   // The phone builds have no game to start here
                    builds.push_back({ entry.path().filename().string(), entry.path() });
        }
        std::sort(builds.begin(), builds.end(), [](const Build& a, const Build& b) { return a.name < b.name; });
        return builds;
    }

    bool copy_data(const fs::path& projectFolder, const Build& build, std::string& error)
    {
        std::error_code fsError;
        const fs::path dataFolder = build.folder / "game_data";
        fs::create_directories(dataFolder, fsError);

        for (const auto& entry : fs::directory_iterator(projectFolder / "game_data", fsError))
        {
            if (!entry.is_regular_file() || entry.path().extension() != ".json") continue;
            if (entry.path().filename() == "player.json") continue; // The build keeps its own progress

            fs::copy_file(entry.path(), dataFolder / entry.path().filename(), fs::copy_options::overwrite_existing, fsError);
            if (fsError)
            {
                error = "Can't copy " + entry.path().filename().string() + " to " + build.name + ": " + fsError.message();
                return false;
            }
        }

        // New and changed images (update_existing only copies files newer than the build's)
        fs::copy(projectFolder / "assets", build.folder / "assets",
            fs::copy_options::recursive | fs::copy_options::update_existing, fsError);
        if (fsError)
        {
            error = "Can't copy the assets to " + build.name + ": " + fsError.message();
            return false;
        }
        return true;
    }

    bool copy_file(const fs::path& projectFolder, const Build& build, const std::string& relativePath, std::string& error)
    {
        std::error_code fsError;
        const fs::path destination = build.folder / relativePath;
        fs::create_directories(destination.parent_path(), fsError);
        fs::copy_file(projectFolder / relativePath, destination, fs::copy_options::overwrite_existing, fsError);
        if (fsError)
        {
            error = "Can't copy " + relativePath + " to " + build.name + ": " + fsError.message();
            return false;
        }
        return true;
    }

    bool launch(const Build& build, std::string& error)
    {
        const fs::path executable = build.folder / GAME_EXECUTABLE;
        const HINSTANCE result = ShellExecuteW(nullptr, L"open", executable.wstring().c_str(), nullptr,
            build.folder.wstring().c_str(), SW_SHOWNORMAL);

        if (reinterpret_cast<INT_PTR>(result) <= 32)
        {
            error = "Can't start " + executable.string();
            return false;
        }
        return true;
    }
}
