#pragma once

#include "EditorPanel.h"
#include "../EditorContext.h"
#include "../EditorWidgets.h"

#include <SFML/Graphics/Texture.hpp>

#include <filesystem>
#include <string>

/*
 * ContentPanel
 * ------------
 * Everything in the project, by tab: units, enemies, stages, chapters, bases, backgrounds, statuses.
 * Clicking an item selects it. Each tab can create new items (from a sprite sheet for units and
 * enemies, by importing a .png for images), duplicate or delete them.
 */
class ContentPanel : public EditorPanel
{
public:
    const char* get_name() const override { return "Content"; }
    void draw(EditorContext& ctx) override;

private:
    // ----- Lists -----
    void draw_units(EditorContext& ctx);
    void draw_enemies(EditorContext& ctx);
    void draw_stages(EditorContext& ctx);
    void draw_chapters(EditorContext& ctx);
    void draw_images(EditorContext& ctx);   // The images of m_imageFolder
    void draw_statuses(EditorContext& ctx);

    // A selectable row: image, title, grey subtitle. True when clicked
    bool row(const char* id, const sf::Texture* texture, EditorWidgets::FrameUV uv,
        const std::string& title, const std::string& subtitle, bool isSelected);
    bool matches_search(const std::string& text, int uid) const;

    // ----- Toolbar -----
    void draw_toolbar(EditorContext& ctx, ContentKind kind);
    void create_stage(EditorContext& ctx);
    void create_chapter(EditorContext& ctx);
    void duplicate(EditorContext& ctx, const Selection& item);

    // ----- New unit / enemy from a sprite sheet -----
    void open_new_entity(ContentKind kind);
    void draw_new_entity_modal(EditorContext& ctx);
    void draw_sheet_slices(const sf::Texture& sheet, int frameCount, int knockbackFrame, float width);
    bool create_entity(EditorContext& ctx);

    // ----- Delete -----
    void draw_delete_modal(EditorContext& ctx);

private:
    std::string m_search;
    ContentKind m_visibleTab = ContentKind::UNIT;
    ImageFolder m_imageFolder = ImageFolder::BASES;   // Folder shown in the Images tab
    Selection m_lastSelection;          // To follow selections made elsewhere (switch tab, scroll to it)
    bool m_isFollowingSelection = false;

    struct NewEntity
    {
        ContentKind kind = ContentKind::NONE;   // UNIT or ENEMY
        std::string name;
        std::filesystem::path sheetPath;
        sf::Texture sheet;
        bool isSheetLoaded = false;
        int frameCount = 1;
        int knockbackFrame = 1;
        bool isMakingIcon = true;               // Units: deploy icon made from the first frame
        std::filesystem::path iconPath;         // Units: or this icon file
        std::string error;
    };
    NewEntity m_new;
    bool m_isNewEntityRequested = false;

    Selection m_toDelete;
    bool m_isDeleteRequested = false;
};
