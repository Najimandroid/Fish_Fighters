#include "EditorApp.h"
#include "Validator.h"

#include <algorithm>
#include <iostream>
#include <string>

namespace
{
    /*
     * FishEditor --check [project]
     * Checks the project like the Validation panel and prints the report, without opening the editor.
     * Returns 1 if the game can't load the data or there are errors (usable in a script or CI).
     */
    int check(const std::filesystem::path& projectFolder)
    {
        ProjectData project(projectFolder);
        if (!project.load())
        {
            std::cout << "Can't open the project: " << project.get_error() << "\n";
            return 1;
        }

        TextureCache textures(projectFolder);
        const ValidationReport report = Validator::run(project, textures);

        std::cout << (report.isLoadedByGame ? "The game loads this data." : "The game can't load this data.") << "\n";
        for (const ValidationIssue& issue : report.issues)
            std::cout << (issue.isError ? "  ERROR  " : "  WARN   ") << issue.message << "\n";
        std::cout << report.count(true) << " error(s), " << report.count(false) << " warning(s)\n";

        return (report.isLoadedByGame && report.count(true) == 0) ? 0 : 1;
    }

    /*
     * "unit:1", "unit:1:2" (form 2), "enemy:3", "stage:-1", "chapter:1", "status:WEAKEN",
     * "base:assets/..." or "background:assets/..."
     */
    Selection parse_selection(const std::string& text)
    {
        const size_t colon = text.find(':');
        if (colon == std::string::npos) return {};

        const std::string kind = text.substr(0, colon);
        const std::string value = text.substr(colon + 1);

        if (kind == "status") return { ContentKind::STATUS, 0, 0, value };
        if (kind == "base") return { ContentKind::BASE, 0, 0, value };
        if (kind == "background") return { ContentKind::BACKGROUND, 0, 0, value };

        try
        {
            const size_t formColon = value.find(':');
            const int uid = std::stoi(value.substr(0, formColon));
            const int form = formColon == std::string::npos ? 0 : std::stoi(value.substr(formColon + 1)) - 1;

            if (kind == "unit") return { ContentKind::UNIT, uid, std::max(0, form) };
            if (kind == "enemy") return { ContentKind::ENEMY, uid };
            if (kind == "stage") return { ContentKind::STAGE, uid };
            if (kind == "chapter") return { ContentKind::CHAPTER, uid };
        }
        catch (const std::exception&)
        {
        }
        return {};
    }
}

/*
 * FishEditor
 * ----------
 * Opens this project's game data (FISH_PROJECT_DIR, set by CMake),
 * or the project folder given as an argument.
 */
int main(int argc, char* argv[])
{
    std::filesystem::path projectFolder = FISH_PROJECT_DIR;
    bool isChecking = false;
    Selection selection;

    for (int i = 1; i < argc; ++i)
    {
        const std::string argument = argv[i];
        if (argument == "--check") isChecking = true;
        else if (argument == "--select" && i + 1 < argc) selection = parse_selection(argv[++i]);
        else projectFolder = argument;
    }

    if (isChecking) return check(projectFolder);

    EditorApp app(projectFolder);
    app.select(selection);
    return app.run();
}
