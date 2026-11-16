#include "MdpImport.h"

#include "../external/nlohmann/json.hpp"

#include <cstdio>

namespace fs = std::filesystem;

namespace
{
    // Where the tool writes (wiped of the previous results first)
    fs::path work_folder()
    {
        std::error_code error;
        const fs::path folder = fs::temp_directory_path(error) / "FishEditor" / "mdp";
        fs::create_directories(folder, error);
        for (const char* name : { "sheet.png", "icon.png" }) fs::remove(folder / name, error);
        return folder;
    }

    std::wstring quoted(const fs::path& path) { return L"\"" + fs::path(path).make_preferred().wstring() + L"\""; }

    // Runs node on the tool with these arguments and returns the JSON line it prints (its last line starting with {)
    std::optional<nlohmann::json> run_tool(const fs::path& projectFolder, const std::wstring& arguments, std::string& error)
    {
        const fs::path tool = projectFolder / "tools" / "art" / "mdp_to_sheet.js";
        if (!fs::exists(tool))
        {
            error = "Can't find " + tool.string();
            return std::nullopt;
        }

        // cmd /c ""node" "tool" ... 2>&1": the outer quotes keep the paths' own quotes (wide, for any file name)
        const std::wstring command = L"\"node " + quoted(tool) + L" " + arguments + L" --json 2>&1\"";
        FILE* pipe = _wpopen(command.c_str(), L"r");
        if (!pipe)
        {
            error = "Can't start Node.js (needed to read .mdp files)";
            return std::nullopt;
        }
        std::string output, line;
        char buffer[1024];
        while (std::fgets(buffer, sizeof(buffer), pipe)) output += buffer;
        const int exitCode = _pclose(pipe);

        std::optional<nlohmann::json> result;
        size_t start = 0;
        while (start < output.size())
        {
            size_t end = output.find('\n', start);
            if (end == std::string::npos) end = output.size();
            line = output.substr(start, end - start);
            if (!line.empty() && line.front() == '{')
            {
                const nlohmann::json parsed = nlohmann::json::parse(line, nullptr, false);
                if (!parsed.is_discarded()) result = parsed;
            }
            start = end + 1;
        }

        if (result && result->contains("error"))
        {
            error = result->at("error").get<std::string>();
            return std::nullopt;
        }
        if (!result || exitCode != 0)
        {
            error = output.empty() ? "Node.js didn't answer (is it installed?)" : output.substr(0, 300);
            return std::nullopt;
        }
        return result;
    }
}

std::optional<MdpImport::Result> MdpImport::convert(const fs::path& projectFolder, const fs::path& drawing, std::string& error, bool isEnemy)
{
    const fs::path folder = work_folder();
    Result result;
    result.sheet = folder / "sheet.png";
    result.icon = folder / "icon.png";

    const auto json = run_tool(projectFolder, quoted(drawing) + L" --sheet " + quoted(result.sheet) + L" --icon " + quoted(result.icon)
        + (isEnemy ? L" --enemy" : L""), error);
    if (!json) return std::nullopt;

    result.frameCount = json->value("frameCount", 0);
    result.knockbackFrameIndex = json->value("knockbackFrameIndex", 0);
    result.walk = json->value("walk", 0);
    result.attack = json->value("attack", 0);
    result.frameWidth = json->value("frameWidth", 0);
    result.frameHeight = json->value("frameHeight", 0);
    if (json->contains("warnings"))
        for (const auto& note : json->at("warnings")) result.notes.push_back(note.get<std::string>());

    std::error_code fsError;
    if (result.frameCount <= 0 || !fs::exists(result.sheet, fsError) || !fs::exists(result.icon, fsError))
    {
        error = "The tool didn't make the sheet and the icon";
        return std::nullopt;
    }
    return result;
}

std::optional<fs::path> MdpImport::icon_from_sheet(const fs::path& projectFolder, const fs::path& sheet, int frameCount, std::string& error,
    bool isEnemy)
{
    const fs::path icon = work_folder() / "icon.png";
    if (!run_tool(projectFolder, L"--icon-from-sheet " + quoted(sheet) + L" --frames " + std::to_wstring(frameCount) + L" --icon " + quoted(icon)
        + (isEnemy ? L" --enemy" : L""), error))
        return std::nullopt;

    std::error_code fsError;
    if (!fs::exists(icon, fsError))
    {
        error = "The tool didn't make the icon";
        return std::nullopt;
    }
    return icon;
}
