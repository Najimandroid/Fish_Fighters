#pragma once

#include "EditorContext.h"

#include "imgui.h"

#include <SFML/Graphics/Texture.hpp>

#include <string>
#include <vector>

/*
 * EditorWidgets
 * -------------
 * ImGui widgets bound to the JSON documents, shared by every editor.
 *
 * Property fields are drawn as rows of a two-column table (name | value), between
 * begin_properties() and end_properties(). They change the document live and commit the
 * undo step when the edit is finished (mouse released, Enter pressed, focus lost).
 */
namespace EditorWidgets
{
    // ===== JSON values =====
    int read_int(const ordered_json& object, const char* key, int fallback = 0);
    double read_number(const ordered_json& object, const char* key, double fallback = 0.0);
    std::string read_string(const ordered_json& object, const char* key, const std::string& fallback = "");
    ordered_json number_value(double value);   // Rounded to 4 decimals, whole numbers stay integers ("2", not "2.0")

    // ===== Layout =====
    bool begin_properties(const char* id);
    void end_properties();
    void property_label(const char* label, const char* help = nullptr);   // Starts a row, the value widget follows
    void section(const char* title);                                       // Titled separator between groups of fields
    void help_marker(const char* text);
    void badge(const char* text, ImU32 color);                             // Small coloured label (types...)

    // ===== Property fields (return true when the value changed) =====
    bool int_field(EditorContext& ctx, DataFile file, const char* label, ordered_json& object, const char* key,
        int min, int max, const char* help = nullptr, float speed = 1.f, const char* format = "%d");
    bool number_field(EditorContext& ctx, DataFile file, const char* label, ordered_json& object, const char* key,
        float min, float max, const char* help = nullptr, float speed = 0.1f, const char* format = "%.2f");
    bool text_field(EditorContext& ctx, DataFile file, const char* label, ordered_json& object, const char* key,
        const char* help = nullptr, bool isMultiline = false);
    bool flag_field(EditorContext& ctx, DataFile file, const char* label, ordered_json& object, const char* key,
        const char* help = nullptr);   // Written as 1 / 0 like the existing files
    bool attack_type_field(EditorContext& ctx, DataFile file, ordered_json& object);
    bool pair_field(EditorContext& ctx, DataFile file, const char* label, ordered_json& object, const char* key,
        float defaultFirst, float defaultSecond, const char* help = nullptr);  // [ a, b ] lists (damageZone)

    // Image path with a thumbnail, a picker of the folder's images and an Import... button
    bool image_field(EditorContext& ctx, DataFile file, const char* label, ordered_json& object, const char* key,
        ImageFolder folder, const char* help = nullptr);

    // ===== Images =====
    ImTextureID texture_id(const sf::Texture& texture);
    ImVec2 fit(ImVec2 source, ImVec2 box);                                 // Biggest size of that ratio inside box

    struct FrameUV { ImVec2 uv0{ 0.f, 0.f }; ImVec2 uv1{ 1.f, 1.f }; ImVec2 size{ 1.f, 1.f }; };
    FrameUV frame_uv(const sf::Texture& texture, int frame, int frameCount);   // Frames are side by side, like the game

    // Image centered in a box of that size (a crossed box if the texture is missing)
    void image_in_box(const sf::Texture* texture, ImVec2 box, FrameUV uv = {}, bool hasBorder = false);
    void frame_in_box(const sf::Texture* texture, int frame, int frameCount, ImVec2 box);

    // Same, drawn into a draw list (tiles, maps)
    void draw_image_in_rect(ImDrawList* drawList, const sf::Texture* texture, ImVec2 min, ImVec2 max, FrameUV uv = {});

    // Clickable tile: image on top, caption below
    bool tile(const char* id, const sf::Texture* texture, FrameUV uv, const std::string& caption, bool isSelected, ImVec2 size);

    // ===== Project images =====
    const sf::Texture* unit_texture(EditorContext& ctx, int uid, int form, FrameUV& uv);   // Icon, or the first frame
    const sf::Texture* enemy_texture(EditorContext& ctx, int uid, FrameUV& uv);            // First frame of the sheet
    void unit_icon(EditorContext& ctx, int uid, int form, ImVec2 box);
    void enemy_icon(EditorContext& ctx, int uid, ImVec2 box);

    // ===== Status icons (STATUS_ICONS_TEXTURE, square cells read left to right) =====
    const sf::Texture* status_icon(EditorContext& ctx, int iconIndex, FrameUV& uv);   // nullptr if no sheet or index < 0
    int status_icon_index(const ProjectData& project, const std::string& statusKey);  // "iconIndex" in statuses.json, -1 if none
    int status_icon_count(EditorContext& ctx);                                         // Cells in the sheet

    // ===== Pickers (popups opened with ImGui::OpenPopup(id)). True when something was picked =====
    bool enemy_picker(EditorContext& ctx, const char* popupId, int& uid);
    bool unit_picker(EditorContext& ctx, const char* popupId, int& uid);
    bool image_picker(EditorContext& ctx, const char* popupId, ImageFolder folder, std::string& path);

    // ===== Types and abilities =====
    // Enemy types (SHELLED, CRUSTACEAN). FISH is hidden: units have it, enemies can't
    void types_field(EditorContext& ctx, DataFile file, ordered_json& object);

    // Abilities list. Enemies always target every fish; units choose the enemy types they target
    void abilities_field(EditorContext& ctx, DataFile file, ordered_json& object, bool isEnemy);
}
