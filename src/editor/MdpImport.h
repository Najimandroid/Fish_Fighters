#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

/*
 * MdpImport
 * ---------
 * Turns a FireAlpaca / MediBang drawing (.mdp) into a sprite sheet and an icon, with tools/art/mdp_to_sheet.js
 * (Node.js needed, like the phone install). In the drawing, each frame is a layer named Walk1, Walk2...,
 * Knockback1, Attack1, Attack2...: the sheet puts them side by side the way the game reads them (walk, knockback,
 * attack) and the icon is the idle frame framed like the hand-made icons (mirrored to face right, zoomed, the head
 * near the right edge). The files are written in a temporary folder, for the AssetLibrary to copy in.
 */
namespace MdpImport
{
    struct Result
    {
        std::filesystem::path sheet;   // The sheet and the icon, in the temporary folder
        std::filesystem::path icon;
        int frameCount = 0;            // What the entity needs: frames in the sheet, the knockback frame's index
        int knockbackFrameIndex = 0;
        int walk = 0, attack = 0;
        int frameWidth = 0, frameHeight = 0;
        std::vector<std::string> notes; // The tool's warnings (empty frames, gaps in the numbers...)
    };

    // The sheet and the icon of a drawing. Empty (and the error) if it couldn't be converted
    std::optional<Result> convert(const std::filesystem::path& projectFolder, const std::filesystem::path& drawing, std::string& error);

    // Only the icon, from the first frame of a sheet (the same framing). Empty (and the error) if it failed
    std::optional<std::filesystem::path> icon_from_sheet(const std::filesystem::path& projectFolder, const std::filesystem::path& sheet,
        int frameCount, std::string& error);
}
