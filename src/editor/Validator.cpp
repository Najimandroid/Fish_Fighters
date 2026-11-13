#include "Validator.h"

#include "AssetLibrary.h"
#include "EditorWidgets.h"
#include "ProjectQueries.h"

#include "../core/DataLoader.h"

#include <fstream>
#include <set>

namespace fs = std::filesystem;
using EditorWidgets::read_int;
using EditorWidgets::read_string;
using EditorWidgets::read_number;

namespace
{
    // ==================
    // Game loader
    // ==================
    void check_with_game_loader(const ProjectData& project, ValidationReport& report)
    {
        const fs::path folder = fs::temp_directory_path() / "FishEditorValidation";
        std::error_code error;
        fs::create_directories(folder, error);

        for (int i = 0; i < static_cast<int>(DataFile::COUNT); ++i)
        {
            const DataFile file = static_cast<DataFile>(i);
            std::ofstream output(folder / ProjectData::file_name(file), std::ios::trunc);
            output << project.get(file).dump(4);
        }

        // The loader also reads the player's save
        fs::copy_file(project.get_data_folder() / "player.json", folder / "player.json", fs::copy_options::overwrite_existing, error);

        DataLoader loader;
        loader.set_data_folder(folder.string());
        loader.set_error_log_enabled(false);

        report.isLoadedByGame = loader.load_all();
        if (!report.isLoadedByGame)
        {
            report.loaderError = loader.get_last_error().empty() ? "The game can't load the data" : loader.get_last_error();
            report.issues.push_back({ true, report.loaderError, {} });
        }
    }

    // ==================
    // Helpers
    // ==================
    void add(ValidationReport& report, bool isError, std::string message, Selection target)
    {
        report.issues.push_back({ isError, std::move(message), std::move(target) });
    }

    // "attackSound": a sound of assets/audios/sfx/sfx.json (optional)
    void check_attack_sound(const AudioLibrary& audio, ValidationReport& report, const ordered_json& entity, const std::string& owner, const Selection& target)
    {
        if (entity.contains("attackSound") && !audio.find_sound(read_string(entity, "attackSound")))
            add(report, false, owner + ": its attack sound \"" + read_string(entity, "attackSound") + "\" isn't in sfx.json (silent hits)", target);
    }

    void check_image(const ProjectData& project, ValidationReport& report, const std::string& path,
        const std::string& owner, const char* what, const Selection& target)
    {
        if (path.empty()) add(report, true, owner + ": no " + what, target);
        else if (!project.exists(path)) add(report, true, owner + ": " + what + " not found (" + path + ")", target);
    }

    // Frame count and knockback frame of a sprite sheet, checked against the image
    void check_sprite(const ProjectData& project, TextureCache& textures, ValidationReport& report,
        const ordered_json& entity, const std::string& owner, const Selection& target)
    {
        const std::string texturePath = read_string(entity, "texture");
        check_image(project, report, texturePath, owner, "sprite sheet", target);

        const int frameCount = read_int(entity, "frameCount", 0);
        const int knockbackFrame = read_int(entity, "knockbackFrameIndex", 0);

        if (frameCount < 1)
        {
            add(report, true, owner + ": the sprite sheet needs at least 1 frame", target);
            return;
        }
        if (knockbackFrame < 1 || knockbackFrame >= frameCount)
            add(report, true, owner + ": the knockback frame must be between 1 and " + std::to_string(frameCount - 1)
                + " (walk frames are before it, attack frames after it)", target);
        else if (knockbackFrame == frameCount - 1)
            add(report, false, owner + ": no attack frames after the knockback frame", target);

        if (const sf::Texture* texture = textures.get(texturePath))
        {
            if (texture->getSize().x % frameCount != 0)
                add(report, false, owner + ": the sheet is " + std::to_string(texture->getSize().x) + " px wide, it doesn't split into "
                    + std::to_string(frameCount) + " equal frames", target);
        }
    }

    // Two entries of a file with the same UID: the game keeps only one of them
    void check_unique_uids(const ProjectData& project, ValidationReport& report, DataFile file, const char* what, ContentKind kind)
    {
        std::set<int> seen;
        for (const auto& [key, entry] : project.get(file).items())
        {
            if (!entry.is_object()) continue;
            const int uid = read_int(entry, "UID", 0);
            if (!seen.insert(uid).second)
                add(report, true, std::string("Two ") + what + " have the UID " + std::to_string(uid) + " (\"" + key + "\"): the game keeps only one", { kind, uid });
        }
    }

    // ==================
    // Units
    // ==================
    void check_units(const ProjectData& project, TextureCache& textures, const AudioLibrary& audio, ValidationReport& report)
    {
        check_unique_uids(project, report, DataFile::UNITS, "units", ContentKind::UNIT);

        for (int uid : project.sorted_uids(DataFile::UNITS))
        {
            const ordered_json& unit = *project.find_uid(DataFile::UNITS, uid);
            if (!unit.contains("forms") || !unit["forms"].is_array() || unit["forms"].empty())
            {
                add(report, true, "Unit " + std::to_string(uid) + ": no form", { ContentKind::UNIT, uid });
                continue;
            }

            const ordered_json& forms = unit["forms"];
            int previousMaxLevel = -1;
            for (size_t i = 0; i < forms.size(); ++i)
            {
                const ordered_json& form = forms[i];
                const int formIndex = static_cast<int>(i);
                const Selection target{ ContentKind::UNIT, uid, formIndex };
                const std::string owner = "Unit " + std::to_string(uid) + " '" + ProjectQueries::unit_name(project, uid, formIndex) + "'"
                    + " (form " + std::to_string(i + 1) + ")";

                if (read_int(form, "form", -1) != formIndex)
                    add(report, true, owner + ": its \"form\" number should be " + std::to_string(formIndex), target);

                const int minLevel = read_int(form, "minLevel", 0);
                const int maxLevel = read_int(form, "maxLevel", 0);
                if (maxLevel < minLevel) add(report, true, owner + ": max level is below min level", target);
                if (i > 0 && minLevel != previousMaxLevel + 1)
                    add(report, false, owner + ": starts at level " + std::to_string(minLevel) + ", the previous form ends at "
                        + std::to_string(previousMaxLevel), target);
                previousMaxLevel = maxLevel;

                check_sprite(project, textures, report, form, owner, target);
                check_attack_sound(audio, report, form, owner, target);

                if (!project.exists(AssetLibrary::unit_icon_path(uid, formIndex)))
                    add(report, false, owner + ": no deploy icon (" + AssetLibrary::unit_icon_path(uid, formIndex) + ")", target);
            }
        }
    }

    // ==================
    // Enemies
    // ==================
    void check_enemies(const ProjectData& project, TextureCache& textures, const AudioLibrary& audio, ValidationReport& report)
    {
        check_unique_uids(project, report, DataFile::ENEMIES, "enemies", ContentKind::ENEMY);

        for (int uid : project.sorted_uids(DataFile::ENEMIES))
        {
            const ordered_json& enemy = *project.find_uid(DataFile::ENEMIES, uid);
            const std::string owner = "Enemy " + std::to_string(uid) + " '" + ProjectQueries::enemy_name(project, uid) + "'";
            check_sprite(project, textures, report, enemy, owner, { ContentKind::ENEMY, uid });
            check_attack_sound(audio, report, enemy, owner, { ContentKind::ENEMY, uid });
        }
    }

    // ==================
    // Stages
    // ==================
    void check_stages(const ProjectData& project, const AudioLibrary& audio, ValidationReport& report)
    {
        check_unique_uids(project, report, DataFile::STAGES, "stages", ContentKind::STAGE);

        for (int uid : project.sorted_uids(DataFile::STAGES))
        {
            const ordered_json& stage = *project.find_uid(DataFile::STAGES, uid);
            const Selection target{ ContentKind::STAGE, uid };
            const std::string owner = "Stage " + std::to_string(uid) + " '" + ProjectQueries::stage_name(project, uid) + "'";

            check_image(project, report, read_string(stage, "baseTexture"), owner, "enemy base image", target);
            check_image(project, report, read_string(stage, "backgroundTexture"), owner, "background", target);

            // Music: names of assets/audios/music/music.json
            if (stage.contains("music") && !audio.find_music(read_string(stage, "music")))
                add(report, false, owner + ": its theme \"" + read_string(stage, "music") + "\" isn't in music.json (no music in that battle)", target);
            if (stage.contains("musicChange"))
            {
                const ordered_json& change = stage["musicChange"];
                if (!change.is_object() || !audio.find_music(read_string(change, "music")))
                    add(report, false, owner + ": the theme it changes to isn't in music.json", target);
                else if (read_number(change, "baseHealth", 50.0) <= 0.0 || read_number(change, "baseHealth", 50.0) > 100.0)
                    add(report, false, owner + ": the theme changes at an enemy base health outside 1-100%", target);
            }

            if (stage.contains("enemies") && stage["enemies"].is_array())
            {
                int wave = 1;
                for (const auto& enemy : stage["enemies"])
                {
                    const int enemyUid = read_int(enemy, "UID", 0);
                    if (!project.find_uid(DataFile::ENEMIES, enemyUid))
                        add(report, true, owner + ": wave " + std::to_string(wave) + " spawns enemy " + std::to_string(enemyUid) + ", which doesn't exist", target);
                    if (read_int(enemy, "amount", 0) == 0)
                        add(report, false, owner + ": wave " + std::to_string(wave) + " spawns 0 enemies", target);
                    ++wave;
                }
                if (stage["enemies"].empty() && uid >= 0) add(report, false, owner + ": no enemy wave", target);
            }

            if (stage.contains("rewards") && stage["rewards"].is_array())
            {
                for (const auto& reward : stage["rewards"])
                {
                    const std::string type = read_string(reward, "type");
                    if (type == "unit" && !project.find_uid(DataFile::UNITS, read_int(reward, "UID", 0)))
                        add(report, true, owner + ": rewards unit " + std::to_string(read_int(reward, "UID", 0)) + ", which doesn't exist", target);
                    else if (type != "unit" && type != "shells" && type != "clamRoom")
                        add(report, true, owner + ": unknown reward type \"" + type + "\"", target);
                }
            }
        }
    }

    // ==================
    // Chapters
    // ==================
    void check_chapters(const ProjectData& project, ValidationReport& report)
    {
        check_unique_uids(project, report, DataFile::CHAPTERS, "chapters", ContentKind::CHAPTER);

        for (int uid : project.sorted_uids(DataFile::CHAPTERS))
        {
            const ordered_json& chapter = *project.find_uid(DataFile::CHAPTERS, uid);
            const Selection target{ ContentKind::CHAPTER, uid };
            const std::string owner = "Chapter " + std::to_string(uid) + " '" + ProjectQueries::chapter_name(project, uid) + "'";

            check_image(project, report, read_string(chapter, "mapTexture"), owner, "map image", target);

            if (!chapter.contains("stages") || !chapter["stages"].is_array() || chapter["stages"].empty())
            {
                add(report, false, owner + ": no stage on the map", target);
                continue;
            }
            for (const auto& point : chapter["stages"])
            {
                const int stageUid = read_int(point, "UID", 0);
                if (!project.find_uid(DataFile::STAGES, stageUid))
                    add(report, true, owner + ": the map shows stage " + std::to_string(stageUid) + ", which doesn't exist", target);
            }
        }
    }

    // ==================
    // Backgrounds (backgrounds.json, keyed by image path)
    // ==================
    void check_backgrounds(const ProjectData& project, ValidationReport& report)
    {
        for (const auto& [path, background] : project.get(DataFile::BACKGROUNDS).items())
        {
            if (!project.exists(path))
                add(report, false, "backgrounds.json has settings for " + path + ", which doesn't exist (renamed or deleted?)", {});
        }
    }
}

namespace Validator
{
    ValidationReport run(const ProjectData& project, TextureCache& textures, const AudioLibrary& audio)
    {
        ValidationReport report;
        report.hasRun = true;

        check_with_game_loader(project, report);
        check_units(project, textures, audio, report);
        check_enemies(project, textures, audio, report);
        check_stages(project, audio, report);
        check_chapters(project, report);
        check_backgrounds(project, report);

        return report;
    }
}
