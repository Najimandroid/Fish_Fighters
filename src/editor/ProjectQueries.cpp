#include "ProjectQueries.h"

namespace
{
    std::string string_of(const ordered_json& object, const char* key)
    {
        auto it = object.find(key);
        return (it != object.end() && it->is_string()) ? it->get<std::string>() : "";
    }

    int int_of(const ordered_json& object, const char* key, int fallback)
    {
        auto it = object.find(key);
        return (it != object.end() && it->is_number()) ? static_cast<int>(it->get<double>()) : fallback;
    }

    // Calls `visit(uid, entry)` for every entry with a UID
    template<typename Visitor>
    void for_each_entry(const ProjectData& project, DataFile file, Visitor visit)
    {
        for (const auto& [key, entry] : project.get(file).items())
        {
            if (!entry.is_object()) continue;
            visit(int_of(entry, "UID", 0), entry);
        }
    }

    std::string stage_label(const ProjectData& project, int uid)
    {
        return "Stage " + std::to_string(uid) + " '" + ProjectQueries::stage_name(project, uid) + "'";
    }
}

namespace ProjectQueries
{
    // ==================
    // Names
    // ==================
    std::string unit_name(const ProjectData& project, int uid, int form)
    {
        const ordered_json* unit = project.find_uid(DataFile::UNITS, uid);
        if (unit && unit->contains("forms") && (*unit)["forms"].is_array())
        {
            const ordered_json& forms = (*unit)["forms"];
            if (form >= 0 && form < static_cast<int>(forms.size()))
            {
                std::string name = string_of(forms[form], "name");
                if (!name.empty()) return name;
            }
        }
        return "Unit #" + std::to_string(uid);
    }

    std::string enemy_name(const ProjectData& project, int uid)
    {
        const ordered_json* enemy = project.find_uid(DataFile::ENEMIES, uid);
        std::string name = enemy ? string_of(*enemy, "name") : "";
        return name.empty() ? "Enemy #" + std::to_string(uid) : name;
    }

    std::string stage_name(const ProjectData& project, int uid)
    {
        const ordered_json* stage = project.find_uid(DataFile::STAGES, uid);
        std::string name = stage ? string_of(*stage, "stageName") : "";
        return name.empty() ? "Stage #" + std::to_string(uid) : name;
    }

    std::string chapter_name(const ProjectData& project, int uid)
    {
        const ordered_json* chapter = project.find_uid(DataFile::CHAPTERS, uid);
        std::string name = chapter ? string_of(*chapter, "name") : "";
        return name.empty() ? "Chapter #" + std::to_string(uid) : name;
    }

    // ==================
    // Uses
    // ==================
    std::vector<Reference> uses_of_unit(const ProjectData& project, int uid)
    {
        std::vector<Reference> uses;
        for_each_entry(project, DataFile::STAGES, [&](int stageUid, const ordered_json& stage)
            {
                if (!stage.contains("rewards") || !stage["rewards"].is_array()) return;
                for (const auto& reward : stage["rewards"])
                {
                    if (string_of(reward, "type") == "unit" && int_of(reward, "UID", 0) == uid)
                        uses.push_back({ stage_label(project, stageUid) + ": reward", { ContentKind::STAGE, stageUid } });
                }
            });
        return uses;
    }

    std::vector<Reference> uses_of_enemy(const ProjectData& project, int uid)
    {
        std::vector<Reference> uses;
        for_each_entry(project, DataFile::STAGES, [&](int stageUid, const ordered_json& stage)
            {
                if (!stage.contains("enemies") || !stage["enemies"].is_array()) return;
                int wave = 1;
                for (const auto& enemy : stage["enemies"])
                {
                    if (int_of(enemy, "UID", 0) == uid)
                        uses.push_back({ stage_label(project, stageUid) + ": wave " + std::to_string(wave), { ContentKind::STAGE, stageUid } });
                    ++wave;
                }
            });
        return uses;
    }

    std::vector<Reference> uses_of_stage(const ProjectData& project, int uid)
    {
        std::vector<Reference> uses;
        for_each_entry(project, DataFile::CHAPTERS, [&](int chapterUid, const ordered_json& chapter)
            {
                if (!chapter.contains("stages") || !chapter["stages"].is_array()) return;
                for (const auto& point : chapter["stages"])
                {
                    if (int_of(point, "UID", 0) == uid)
                        uses.push_back({ "Chapter " + std::to_string(chapterUid) + " '" + chapter_name(project, chapterUid) + "'", { ContentKind::CHAPTER, chapterUid } });
                }
            });
        return uses;
    }

    std::vector<Reference> uses_of_image(const ProjectData& project, const std::string& path)
    {
        std::vector<Reference> uses;

        for_each_entry(project, DataFile::UNITS, [&](int uid, const ordered_json& unit)
            {
                if (!unit.contains("forms") || !unit["forms"].is_array()) return;
                for (size_t form = 0; form < unit["forms"].size(); ++form)
                {
                    if (string_of(unit["forms"][form], "texture") == path)
                        uses.push_back({ "Unit " + std::to_string(uid) + " '" + unit_name(project, uid, static_cast<int>(form)) + "': sprite sheet",
                            { ContentKind::UNIT, uid, static_cast<int>(form) } });
                }
            });

        for_each_entry(project, DataFile::ENEMIES, [&](int uid, const ordered_json& enemy)
            {
                if (string_of(enemy, "texture") == path)
                    uses.push_back({ "Enemy " + std::to_string(uid) + " '" + enemy_name(project, uid) + "': sprite sheet", { ContentKind::ENEMY, uid } });
            });

        for_each_entry(project, DataFile::STAGES, [&](int uid, const ordered_json& stage)
            {
                if (string_of(stage, "baseTexture") == path)
                    uses.push_back({ stage_label(project, uid) + ": enemy base", { ContentKind::STAGE, uid } });
                if (string_of(stage, "backgroundTexture") == path)
                    uses.push_back({ stage_label(project, uid) + ": background", { ContentKind::STAGE, uid } });
            });

        for_each_entry(project, DataFile::CHAPTERS, [&](int uid, const ordered_json& chapter)
            {
                if (string_of(chapter, "mapTexture") == path)
                    uses.push_back({ "Chapter " + std::to_string(uid) + " '" + chapter_name(project, uid) + "': map", { ContentKind::CHAPTER, uid } });
            });

        return uses;
    }
}
