#pragma once

#include <sstream>
#include <string>

/*
 * Platform
 * --------
 * What differs between the PC and the phone builds, in one place.
 *
 *   - Files the game reads (game_data/*.json, music.json, sfx.json): on PC they're next to the executable; on Android
 *     they're inside the APK, which only SFML's streams can read. open() reads either (a saved copy first).
 *   - Files the game writes (the save, the options, the error log): on PC next to the executable as before; on Android
 *     in the app's private storage. writable() gives the path to write to.
 *   - IS_MOBILE: the phone build (touch, the phone layout). On PC, FISH_MOBILE_PREVIEW turns the phone layout on to
 *     test it (CMake preset pc-mobile-preview).
 *
 * Example usage:
 *   auto file = Platform::open("game_data/units.json");
 *   if (!file) return false;
 *   json j; file >> j;
 *   std::ofstream save(Platform::writable("game_data/player.json"));
 */
namespace Platform
{
#if defined(__ANDROID__) || defined(FISH_MOBILE_PREVIEW)
    inline constexpr bool IS_MOBILE = true;
#else
    inline constexpr bool IS_MOBILE = false;
#endif

#if defined(__ANDROID__)
    inline constexpr bool IS_ANDROID = true;
#else
    inline constexpr bool IS_ANDROID = false;
#endif

    // The file's whole text as a stream (false when it can't be read): the writable copy if there is one (a save),
    // else the game's own file (the APK's on Android)
    std::stringstream open(const std::string& path);

    // Where to write a file the game keeps (its folders are made): the path itself on PC, inside the app's private
    // storage on Android
    std::string writable(const std::string& path);

    // Whether the game has this file (an image, a sound...), on PC or in the APK
    bool exists(const std::string& path);
}
