#include "EntityFields.h"

#include "../EditorWidgets.h"

#include <algorithm>

using namespace EditorWidgets;

namespace EntityFields
{
    void stats(EditorContext& ctx, DataFile file, ordered_json& entity)
    {
        int_field(ctx, file, "Health", entity, "health", 1, 1000000000, nullptr, 1.f);
        int_field(ctx, file, "Attack power", entity, "attackPower", 0, 1000000000,
            "Damage of one hit, before abilities and level / stage multipliers.");
        int_field(ctx, file, "Knockbacks", entity, "knockbackCount", 0, 100,
            "How many times it's pushed back as it loses health.", 0.05f);
        number_field(ctx, file, "Movement speed", entity, "movementSpeed", 0.f, 1000.f, nullptr, 0.1f, "%.1f");
    }

    void attack(EditorContext& ctx, DataFile file, ordered_json& entity)
    {
        attack_type_field(ctx, file, entity);
        number_field(ctx, file, "Attack range", entity, "attackRange", 0.f, 5000.f,
            "It stops walking and attacks when a target is this close.", 0.5f, "%.1f");

        const float range = static_cast<float>(read_number(entity, "attackRange"));
        pair_field(ctx, file, "Damage zone", entity, "damageZone", 0.f, range,
            "Start and end of the zone the attack hits, from its position. Without it, the game uses 0 to the attack range.");

        number_field(ctx, file, "Time between attacks", entity, "attackFrequency", 0.01f, 600.f,
            "Seconds between two attacks (\"attackFrequency\" in the JSON).", 0.05f, "%.2f s");
        number_field(ctx, file, "Foreswing", entity, "foreswing", 0.f, 60.f,
            "Wind-up: seconds between the start of the attack and the hit.", 0.01f, "%.2f s");
        number_field(ctx, file, "Backswing", entity, "backswing", 0.f, 60.f,
            "Recovery: seconds after the hit before it can move again.", 0.01f, "%.2f s");

        property_label("Damage per second");
        ImGui::TextDisabled("%.1f (one target, without abilities)", damage_per_second(entity));
    }

    void sprite(EditorContext& ctx, DataFile file, ordered_json& entity, ImageFolder folder)
    {
        image_field(ctx, file, "Sprite sheet", entity, "texture", folder,
            "Frames side by side: idle and walk first, then the knockback frame, then the attack.");

        if (int_field(ctx, file, "Frames", entity, "frameCount", 1, 256,
            "Number of frames in the sheet. Frame width = sheet width / frames.", 0.05f))
        {
            // Keeps the knockback frame inside the sheet
            const int frames = read_int(entity, "frameCount", 1);
            entity["knockbackFrameIndex"] = std::clamp(read_int(entity, "knockbackFrameIndex", 1), 1, std::max(1, frames - 1));
        }

        const int frames = read_int(entity, "frameCount", 1);
        int_field(ctx, file, "Knockback frame", entity, "knockbackFrameIndex", 1, std::max(1, frames - 1),
            "Frames before it: walk. Frames after it: attack.\nYou can also pick it in the Preview panel's frame strip.", 0.05f);
    }

    float damage_per_second(const ordered_json& entity, float multiplier)
    {
        const float frequency = static_cast<float>(read_number(entity, "attackFrequency", 1.0));
        if (frequency <= 0.f) return 0.f;
        return static_cast<float>(read_int(entity, "attackPower")) * multiplier / frequency;
    }
}
