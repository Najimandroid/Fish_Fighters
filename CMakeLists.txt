cmake_minimum_required(VERSION 3.15)
project(Fish_Fighters)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# SFML
set(SFML_DIR "${CMAKE_SOURCE_DIR}/external/SFML-3.0.0/lib/cmake/SFML")
find_package(SFML 3 REQUIRED COMPONENTS Graphics Audio Window System)

# ---------------------------------------------------------------------
# Source files grouped par cible
# ---------------------------------------------------------------------

# ImGui
set(IMGUI_SOURCES
    external/imgui/imgui.cpp
    external/imgui/imgui_demo.cpp
    external/imgui/imgui_draw.cpp
    external/imgui/imgui_tables.cpp
    external/imgui/imgui_widgets.cpp
)

# ImGui-SFML
set(IMGUI_SFML_SOURCES
    external/imgui-sfml/imgui-SFML.cpp
)

# Application sources
set(PROJECT_SOURCES
    src/main.cpp

    src/core/Game.cpp

    src/core/entities/BattleEntity.cpp
    src/core/entities/BattleEnemy.cpp
    src/core/entities/BattleUnit.cpp
    src/core/entities/BattleBase.cpp

    src/core/entities/machine/StateMachine.cpp
    src/core/entities/machine/states/AttackState.cpp
    src/core/entities/machine/states/BaseState.cpp
    src/core/entities/machine/states/DeadState.cpp
    src/core/entities/machine/states/IdleState.cpp
    src/core/entities/machine/states/KnockbackState.cpp
    src/core/entities/machine/states/WalkState.cpp
    src/core/entities/machine/states/ParalyzedState.cpp

    src/core/stages/Stage.cpp

    src/core/combat/DamageCalculator.cpp
    src/core/combat/StatusManager.cpp
    src/core/combat/CombatLog.cpp

    src/uis/UIManager.cpp

    src/uis/widgets/UIElement.cpp
    src/uis/widgets/UIBattleIcon.cpp
    src/uis/widgets/UIButtonElement.cpp
    src/uis/widgets/UITextElement.cpp
    src/uis/widgets/UITextureElement.cpp
    src/uis/widgets/UIDeploymentIcons.cpp
    src/uis/widgets/UIStageCash.cpp
    src/uis/widgets/UICashUp.cpp
    src/uis/widgets/UIBaseHealthInfo.cpp
    src/uis/widgets/UIUpgradeIcon.cpp
    src/uis/widgets/UIUpgradeSlider.cpp
    src/uis/widgets/UIPlayerShellsInfo.cpp
    src/uis/widgets/UIEquipIcon.cpp
    src/uis/widgets/UIEquipSlot.cpp
    src/uis/widgets/UIEquipSlider.cpp
    src/uis/widgets/UIChapterMap.cpp

    # ImGui theme shared by the developer tools and the editor
    src/tools/ToolsTheme.cpp

    # Developer tools (their content is only compiled with DEBUG_MODE)
    src/debug/DebugTools.cpp
    src/debug/DebugWidgets.cpp
    src/debug/panels/PerformancePanel.cpp
    src/debug/panels/StagePanel.cpp
    src/debug/panels/SpawnerPanel.cpp
    src/debug/panels/InspectorPanel.cpp
    src/debug/panels/StatusLogPanel.cpp
    src/debug/panels/CombatLogPanel.cpp
    src/debug/panels/OverlayPanel.cpp
    # ← ajoute ici les futurs fichiers, par ex.:
    # src/utils/NewSystem.cpp
)

# ---------------------------------------------------------------------
# ImGui Library
# ---------------------------------------------------------------------
add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(imgui PUBLIC external/imgui)

# ImGui-SFML Library
add_library(imgui-sfml STATIC ${IMGUI_SFML_SOURCES})
target_include_directories(imgui-sfml PUBLIC external/imgui-sfml external/imgui)
target_link_libraries(imgui-sfml PUBLIC imgui SFML::Graphics SFML::Window SFML::System)

# ---------------------------------------------------------------------
# fishcore: game data shared by the game and the editor
# (JSON loading and validation, data structures, player save)
# ---------------------------------------------------------------------
add_library(fishcore STATIC
    src/core/DataLoader.cpp
    src/data/PlayerData.cpp
)
target_include_directories(fishcore PUBLIC src/ external/SFML-3.0.0/include)
target_link_libraries(fishcore PUBLIC SFML::System)

# ---------------------------------------------------------------------
# Main executable
# ---------------------------------------------------------------------
add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
    external/imgui
    external/imgui-sfml
    external/tweeny/include
    external/SFML-3.0.0/include
    src/
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    fishcore
    imgui-sfml
    SFML::Graphics
    SFML::Audio
    SFML::Window
    SFML::System
    opengl32.lib
)


# ---------------------------------------------------------------------
# FishEditor: the game data editor (units, enemies, stages, chapters, images)
# Edits the project's game_data and assets folders directly
# ---------------------------------------------------------------------
set(EDITOR_SOURCES
    src/editor/main.cpp
    src/editor/EditorApp.cpp
    src/editor/ProjectData.cpp
    src/editor/ProjectQueries.cpp
    src/editor/TextureCache.cpp
    src/editor/AssetLibrary.cpp
    src/editor/FileDialog.cpp
    src/editor/GameBuilds.cpp
    src/editor/Validator.cpp
    src/editor/EditorWidgets.cpp

    src/editor/panels/ContentPanel.cpp
    src/editor/panels/PropertiesPanel.cpp
    src/editor/panels/PreviewPanel.cpp
    src/editor/panels/ValidationPanel.cpp

    src/editor/editors/EntityFields.cpp
    src/editor/editors/UnitEditor.cpp
    src/editor/editors/EnemyEditor.cpp
    src/editor/editors/StageEditor.cpp
    src/editor/editors/ChapterEditor.cpp
    src/editor/editors/ImageAssetEditor.cpp

    src/tools/ToolsTheme.cpp
    external/imgui/misc/cpp/imgui_stdlib.cpp
)

add_executable(FishEditor ${EDITOR_SOURCES})

target_include_directories(FishEditor PRIVATE
    external/imgui
    external/imgui/misc/cpp
    external/imgui-sfml
    external/SFML-3.0.0/include
    src/
)

target_link_libraries(FishEditor PRIVATE
    fishcore
    imgui-sfml
    SFML::Graphics
    SFML::Window
    SFML::System
    opengl32.lib
    comdlg32.lib
)

# The editor opens this project's game_data and assets folders
target_compile_definitions(FishEditor PRIVATE FISH_PROJECT_DIR="${CMAKE_SOURCE_DIR}")

# ---------------------------------------------------------------------
# Debug macro
# ---------------------------------------------------------------------
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(${PROJECT_NAME} PRIVATE DEBUG_MODE)
endif()

# ---------------------------------------------------------------------
# DLLs copy configuration
# ---------------------------------------------------------------------
set(SFML_DLL_DIR "${CMAKE_SOURCE_DIR}/external/SFML-3.0.0/bin")

set(SFML_DLLS_DEBUG
    "${SFML_DLL_DIR}/sfml-graphics-d-3.dll"
    "${SFML_DLL_DIR}/sfml-window-d-3.dll"
    "${SFML_DLL_DIR}/sfml-audio-d-3.dll"
    "${SFML_DLL_DIR}/sfml-system-d-3.dll"
)

set(SFML_DLLS_RELEASE
    "${SFML_DLL_DIR}/sfml-graphics-3.dll"
    "${SFML_DLL_DIR}/sfml-window-3.dll"
    "${SFML_DLL_DIR}/sfml-audio-3.dll"
    "${SFML_DLL_DIR}/sfml-system-3.dll"
)

# The game and the editor both need the SFML DLLs next to them
foreach(target ${PROJECT_NAME} FishEditor)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(TARGET_DLLS ${SFML_DLLS_DEBUG})
    else()
        set(TARGET_DLLS ${SFML_DLLS_RELEASE})
    endif()
    foreach(dll ${TARGET_DLLS})
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${dll}" $<TARGET_FILE_DIR:${target}>
        )
    endforeach()
endforeach()

# ---------------------------------------------------------------------
# Copy assets folder to output
# ---------------------------------------------------------------------
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets
    $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets
)

# ---------------------------------------------------------------------
# Copy game_data folder to output
# ---------------------------------------------------------------------
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/game_data
    $<TARGET_FILE_DIR:${PROJECT_NAME}>/game_data
)



