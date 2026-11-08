#pragma once

#include "EditorContext.h"

#include <string>
#include <vector>

/*
 * ProjectQueries
 * --------------
 * Read-only questions about the project data: names, and who uses what
 * (shown before deleting something, and in the images' "Used by" list).
 */
struct Reference
{
    std::string label;     // "Stage 3 'Seaside Rampage': wave 2"
    Selection target;      // Clicking it selects the user
};

namespace ProjectQueries
{
    // ===== Names (fallback "Unit #3" if the entry or its name is missing) =====
    std::string unit_name(const ProjectData& project, int uid, int form = 0);
    std::string enemy_name(const ProjectData& project, int uid);
    std::string stage_name(const ProjectData& project, int uid);
    std::string chapter_name(const ProjectData& project, int uid);

    // ===== Who uses it =====
    std::vector<Reference> uses_of_unit(const ProjectData& project, int uid);    // Stage rewards
    std::vector<Reference> uses_of_enemy(const ProjectData& project, int uid);   // Stage waves
    std::vector<Reference> uses_of_stage(const ProjectData& project, int uid);   // Chapter maps
    std::vector<Reference> uses_of_image(const ProjectData& project, const std::string& path);

    // ===== Backgrounds =====
    // Row of a stage background units stand on: from the first stage using it that sets one,
    // else the template's ratio of the image height (background_ground_row in StageData.h)
    float background_ground_row(const ProjectData& project, const std::string& path, float imageHeight);
}
