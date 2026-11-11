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
            "It stops walking and attacks when a target's hitbox is closer than this to its center.\n"
            "The Preview panel draws it (yellow).", 0.5f, "%.1f");

        const float range = static_cast<float>(read_number(entity, "attackRange"));
        pair_field(ctx, file, "Damage zone (offset, width)", entity, "damageZone", 0.f, range,
            "Where the attack hits, in front of its center: it starts \"offset\" pixels ahead and is \"width\" pixels long\n"
            "(the game reads the second number as a width). Without it: offset 0, width = attack range.\n"
            "The Preview panel draws it (red).");

        number_field(ctx, file, "Time between attacks", entity, "attackFrequency", 0.01f, 600.f,
            "Seconds between two attacks (\"attackFrequency\" in the JSON).", 0.05f, "%.2f s");
        number_field(ctx, file, "Foreswing", entity, "foreswing", 0.f, 60.f,
            "Wind-up: seconds between the start of the attack and the hit.", 0.01f, "%.2f s");
        number_field(ctx, file, "Backswing", entity, "backswing", 0.f, 60.f,
            "Recovery: seconds after the hit before it can move again.", 0.01f, "%.2f s");
        sound_field(ctx, file, "Attack sound", entity, "attackSound", "attack_", "attack_slap",
            "Played when its attack hits (once per attack, even when it hits several targets).\n"
            "The sounds are in assets/audios/sfx (made by tools/music/make_sfx.js).");

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

        scale_field(ctx, file, "Shadow size", entity, "shadowScale", 0.2f, 3.f,
            "Width of its round shadow in battle, compared to its body (the hitbox): 100 % by default.\n"
            "The Preview panel draws it like the game.");

        optional_number_field(ctx, file, "Shadow offset", entity, "shadowOffset", 0.f, -500.f, 500.f, 1.f, "%.0f px", 1.f,
            "Moves its shadow forward (toward where it faces), or back with a negative number. 0 by default:\n"
            "under its body (the back half of the frame). The Preview panel draws it like the game.");
    }

    float damage_per_second(const ordered_json& entity, float multiplier)
    {
        const float frequency = static_cast<float>(read_number(entity, "attackFrequency", 1.0));
        if (frequency <= 0.f) return 0.f;
        return static_cast<float>(read_int(entity, "attackPower")) * multiplier / frequency;
    }
}

/*
 * The entity's icon (a file found by its name, not stored in the JSON): import one or make it from the first frame
 */
void EntityFields::icon(EditorContext& ctx, const std::string& iconPath, const char* label, const char* help, const ordered_json& entity)
{
    property_label(label, help);

    const sf::Texture* texture = ctx.textures.get(iconPath);

    image_in_box(texture, ImVec2(112.f, 84.f), {}, true);
    ImGui::SameLine();
    ImGui::BeginGroup();
    if (texture) ImGui::TextDisabled("%u x %u", texture->getSize().x, texture->getSize().y);
    else ImGui::TextColored(ImVec4(1.f, 0.45f, 0.4f, 1.f), "No icon yet");

    const ImGuiID importId = ImGui::GetID("ImportIcon");
    if (ImGui::Button("Import...")) ctx.request_png_dialog(importId, "Pick the icon (200 x 150)");

    ImGui::SameLine();
    const sf::Texture* sheet = ctx.textures.get(read_string(entity, "texture"));
    ImGui::BeginDisabled(!sheet);
    if (ImGui::Button("Make from first frame") && sheet)
    {
        std::string error;
        const auto image = AssetLibrary::make_icon(*sheet, read_int(entity, "frameCount", 1));
        if (image && ctx.assets.replace(iconPath, *image, error)) ctx.notify("Icon made from the first frame: " + iconPath);
        else ctx.notify(error.empty() ? "Can't make the icon" : error, true);
        ctx.textures.forget(iconPath);
        ctx.mark_changed();
    }
    ImGui::EndDisabled();
    ImGui::EndGroup();

    if (auto picked = ctx.take_dialog_result(importId))
    {
        std::string error;
        if (ctx.assets.replace(iconPath, *picked, error)) ctx.notify("Icon replaced: " + iconPath);
        else ctx.notify(error, true);
        ctx.textures.forget(iconPath);
        ctx.mark_changed();
    }
}
