# Fish Fighters

Fish Fighters is a personal C++ game project developed without using a game engine.
It focuses on clean architecture, modular systems, and data-driven design.

A full presentation of the project — including gameplay, technical breakdown, and design decisions — is available on my portfolio.

👉 **Project page:**  
https://najimandroid.github.io/projects/project-fishfighters.html

---

## Play the game

No installation or build tools needed.

1. Go to the [v0.1.0 prototype release](https://github.com/Najimandroid/Fish_Fighters/releases/tag/v0.1.0-prototype) and download one of the zips:
   - [**FishFighters-v0.1.0-prototype.zip**](https://github.com/Najimandroid/Fish_Fighters/releases/download/v0.1.0-prototype/FishFighters-v0.1.0-prototype.zip): the game.
   - [**FishFighters-v0.1.0-prototype-devtools.zip**](https://github.com/Najimandroid/Fish_Fighters/releases/download/v0.1.0-prototype/FishFighters-v0.1.0-prototype-devtools.zip): the same game with my in-game debug tools enabled (FPS and frame stepping, stage loading, unit/enemy spawning, hitbox display).
2. Extract the whole folder somewhere you can write to, such as your Desktop. Don't run it from inside the zip.
3. Run `Fish_Fighters.exe`.

Requires Windows 10 or 11 (64-bit). Windows may show a "Windows protected your PC" warning because the game isn't signed: click **More info**, then **Run anyway**.

### Controls

| Input | Action |
|---|---|
| Mouse click | Buttons, deploy units in battle |
| Left mouse drag | Pan the battlefield |
| Mouse wheel | Zoom the battlefield |
| Left / Right arrows | Browse units and stages in menus |
| Hold and drag a unit | Equip it in a slot (Equip menu) |
| Right-click a slot | Unequip it |
| P / Left Shift / R | Pause / 2x speed / normal speed |
| QUIT button or Alt+F4 | Close the game (progress is saved on close) |

The included `README.txt` lists the prototype's known limitations.

---

## About the project

- Genre: Horizontal tower defense  
- Technology: C++20 (custom engine, SFML 3)  
- Scope: Personal learning project  
- Status: Work in progress — [v0.1.0 prototype](https://github.com/Najimandroid/Fish_Fighters/releases/tag/v0.1.0-prototype) released, rework in progress

The project was designed to deepen my understanding of game architecture, state machines, and system-oriented design in C++.

---

## Building from source

Only needed if you want to work on the code.

### Requirements
- Git
- CMake 3.15 or newer
- A C++20 compatible compiler
- Windows (currently the only supported platform)

### Build steps (Windows)

Clone the project:
```bash
git clone https://github.com/Najimandroid/Fish_Fighters.git
```

From the project directory:
```bash
cmake -S . -B build
cmake --build build --config Release
```

The .exe file will be located inside the `build/Release` folder.

---

For more details, screenshots, and explanations, please refer to the portfolio page linked above.
