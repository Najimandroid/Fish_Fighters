#pragma once

/*
 * Settings
 * --------
 * The player's options (Options menu), saved in settings.json next to player.json.
 * A missing or broken file just means the defaults.
 */
struct Settings
{
    // ===== Audio =====
    // Slider positions from 0 to 1. The loudness follows their square, so the slider feels even
    // (a straight line would put most of the audible change in its bottom quarter)
    float masterVolume = 1.f;
    float musicVolume = 1.f;
    float effectsVolume = 1.f;
    bool muteInBackground = true;   // Silent while another window has the focus

    // ===== Display =====
    bool fullscreen = true;         // Borderless at the desktop size; false: a resizable 16:9 window
};
