#pragma once

#include "ProjectData.h"
#include "TextureCache.h"
#include "AssetLibrary.h"

#include "imgui.h"

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Clock.hpp>

#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <vector>

/*
 * ContentKind / Selection
 * -----------------------
 * What is selected in the editor (the Properties and Preview panels show it).
 */
enum class ContentKind
{
    NONE,
    UNIT,
    ENEMY,
    STAGE,
    CHAPTER,
    BASE,        // An image of assets/.../bases
    BACKGROUND,  // An image of assets/.../backgrounds
    STATUS
};

struct Selection
{
    ContentKind kind = ContentKind::NONE;
    int uid = 0;
    int form = 0;          // Units: the form shown
    std::string name;      // Images: their "assets/..." path. Statuses: their key ("WEAKEN")

    bool operator==(const Selection&) const = default;
};

/*
 * ValidationIssue / ValidationReport
 * ----------------------------------
 * Result of the Validator: can the game load the data, and what looks wrong.
 */
struct ValidationIssue
{
    bool isError = true;   // false: warning (the game loads it, but it's probably a mistake)
    std::string message;
    Selection target;      // Clicking the issue selects it
};

struct ValidationReport
{
    bool hasRun = false;
    bool isLoadedByGame = false;
    std::string loaderError;
    std::vector<ValidationIssue> issues;

    int count(bool errors) const
    {
        int total = 0;
        for (const auto& issue : issues) if (issue.isError == errors) ++total;
        return total;
    }
};

/*
 * EditorContext
 * -------------
 * Everything the panels share: the project, its images, the selection, the validation report.
 * Panels never talk to each other: they read and change this context.
 *
 * Rules for the panels:
 *  - Change the JSON values directly, then call commit() once the edit is finished (undo step)
 *  - Structural changes that would break references held by the panel drawn after
 *    (deleting an entry, undo) go through defer(): they run after the frame
 *  - The file dialog blocks, so it's opened between two frames: request_png_dialog(id),
 *    then take_dialog_result(id) returns the picked file on the next frame
 */
class EditorContext
{
public:
    EditorContext(ProjectData& project, TextureCache& textures, AssetLibrary& assets, sf::RenderWindow& window) :
        project(project), textures(textures), assets(assets), window(window)
    {
    }

    ProjectData& project;
    TextureCache& textures;
    AssetLibrary& assets;
    sf::RenderWindow& window;

    Selection selection;
    ValidationReport validation;

    // ===== Edits =====
    void commit(DataFile file)
    {
        project.commit_edit(file);
        mark_changed();
    }

    // Asks for a new validation (it runs once the edits stop for a moment)
    void mark_changed()
    {
        m_isValidationDue = true;
        m_lastChange.restart();
    }

    bool is_validation_due() const { return m_isValidationDue && m_lastChange.getElapsedTime().asSeconds() > 0.35f; }
    void clear_validation_due() { m_isValidationDue = false; }

    void select(Selection newSelection) { selection = std::move(newSelection); }

    // ===== After the frame =====
    void defer(std::function<void()> action) { m_deferred.push_back(std::move(action)); }

    std::vector<std::function<void()>> take_deferred()
    {
        std::vector<std::function<void()>> actions;
        actions.swap(m_deferred);
        return actions;
    }

    // ===== File dialog =====
    void request_png_dialog(ImGuiID requester, std::string title)
    {
        m_dialogRequester = requester;
        m_dialogTitle = std::move(title);
    }

    std::optional<std::filesystem::path> take_dialog_result(ImGuiID requester)
    {
        if (m_dialogResultOwner != requester || !m_dialogResult) return std::nullopt;
        std::optional<std::filesystem::path> result = std::move(m_dialogResult);
        m_dialogResult.reset();
        m_dialogResultOwner = 0;
        return result;
    }

    // Used by the EditorApp between frames
    bool has_dialog_request() const { return m_dialogRequester != 0; }
    const std::string& get_dialog_title() const { return m_dialogTitle; }
    void deliver_dialog_result(std::optional<std::filesystem::path> result)
    {
        m_dialogResultOwner = m_dialogRequester;
        m_dialogResult = std::move(result);
        m_dialogRequester = 0;
        m_dialogResultAge = 0;
    }

    // A result nobody took (the widget isn't drawn anymore) is dropped after a few frames
    void age_dialog_result()
    {
        if (m_dialogResult && ++m_dialogResultAge > 3)
        {
            m_dialogResult.reset();
            m_dialogResultOwner = 0;
        }
    }

    // ===== Status bar =====
    void notify(std::string message, bool isError = false)
    {
        m_statusMessage = std::move(message);
        m_isStatusError = isError;
        m_statusClock.restart();
    }

    const std::string& get_status_message() const { return m_statusMessage; }
    bool is_status_error() const { return m_isStatusError; }
    float get_status_age() const { return m_statusClock.getElapsedTime().asSeconds(); }

private:
    bool m_isValidationDue = true;
    sf::Clock m_lastChange;

    std::vector<std::function<void()>> m_deferred;

    ImGuiID m_dialogRequester = 0;
    std::string m_dialogTitle;
    ImGuiID m_dialogResultOwner = 0;
    std::optional<std::filesystem::path> m_dialogResult;
    int m_dialogResultAge = 0;

    std::string m_statusMessage;
    bool m_isStatusError = false;
    sf::Clock m_statusClock;
};
