cmake_minimum_required(VERSION 3.24)
project(soccerkid C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()
set(SK_VERSION 1.0)
if(MSVC)
  add_compile_options(/W3)
  add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
else()
  add_compile_options(-Wall -Wextra -Wshadow -Wconversion -Wno-sign-conversion)
  if(CMAKE_C_COMPILER_ID MATCHES "Clang")                # warnings are errors where the port is tested (Clang, macOS)
    add_compile_options(-Werror)
  endif()
endif()
add_compile_definitions(_POSIX_C_SOURCE=200809L _DARWIN_C_SOURCE _DEFAULT_SOURCE)

find_package(SDL3 CONFIG REQUIRED)
find_package(ZLIB REQUIRED)

add_library(skcore STATIC
  src/core/err.c
  src/core/bytekiller.c
  src/core/zipfile.c
  src/core/adf.c
  src/core/dirlist.c
  src/core/disks.c
  src/core/hunk.c
  src/core/tables.c
  src/formats/level.c
  src/formats/custom.c
  src/formats/sections.c
  src/formats/bobs.c
  src/formats/screens.c
  src/formats/mapsprites.c
  src/formats/anim.c
  src/gfx/framebuffer.c
  src/gfx/palette.c
  src/gfx/present.c
  src/gfx/png.c
  src/gfx/scale.c
  src/gfx/sprites.c
  src/app/args.c
  src/app/scene.c
  src/app/play.c
  src/app/pictures.c
  src/app/frontend.c
  src/app/savescreen.c
  src/app/intro.c
  src/app/controls.c
  src/app/remap.c
  src/app/window.c
  src/game/world.c
  src/game/collision.c
  src/game/player.c
  src/game/player_anim.c
  src/game/player_states.c
  src/game/objects.c
  src/game/camera.c
  src/game/attrs.c
  src/game/level_flow.c
  src/game/resources.c
  src/game/ball.c
  src/game/player_tricks.c
  src/game/obj_helpers.c
  src/game/obj_england.c
  src/game/attr_england.c
  src/game/hud.c
  src/game/platforms.c
  src/game/obj_london.c
  src/game/obj_country.c
  src/game/obj_italy.c
  src/game/obj_russia.c
  src/game/obj_japan.c
  src/game/attr_country_london.c
  src/game/attr_italy.c
  src/game/attr_russia.c
  src/game/attr_japan.c
  src/game/scroll_modes.c
  src/game/obj_usa.c
  src/game/obj_bonus.c
  src/game/obj_front.c
  src/game/between.c
  src/game/scoreboard.c
  src/game/front.c
  src/game/attract.c
  src/game/savegame.c
  src/game/attr_usa.c
  src/audio/paula.c
  src/audio/ptreplay.c
  src/audio/sound.c
  src/app/audio_out.c)
target_include_directories(skcore PUBLIC src)
target_link_libraries(skcore PUBLIC ZLIB::ZLIB SDL3::SDL3)

# The game data: build/skextract <roms dir> writes data/soccerkid.dat; every build then embeds it in soccerkid (skembed).
add_executable(skextract tools/skextract.c)
target_link_libraries(skextract PRIVATE skcore)
target_compile_definitions(skextract PRIVATE SK_DEFAULT_DATA="${CMAKE_SOURCE_DIR}/data/soccerkid.dat")
add_executable(skembed tools/skembed.c)
set(SK_EMBEDDED_C ${CMAKE_BINARY_DIR}/embedded_data.c)
add_custom_target(embed_data ALL
  COMMAND skembed ${CMAKE_SOURCE_DIR}/data/soccerkid.dat ${SK_EMBEDDED_C}
  BYPRODUCTS ${SK_EMBEDDED_C}
  COMMENT "Embedding data/soccerkid.dat (if present)"
  VERBATIM)
add_dependencies(embed_data skembed)
set_source_files_properties(${SK_EMBEDDED_C} PROPERTIES GENERATED TRUE)
if(NOT MSVC)
  set_source_files_properties(${SK_EMBEDDED_C} PROPERTIES COMPILE_OPTIONS "-w")
endif()

# The level designer's document, checks, names and map drawing (no window), shared by skld and the tests.
add_library(skedit STATIC
  src/editor/doc.c
  src/editor/check.c
  src/editor/names.c
  src/editor/flag_label.c
  src/editor/view.c)
target_include_directories(skedit PUBLIC src)
target_link_libraries(skedit PUBLIC skcore)

add_executable(soccerkid src/app/main.c ${SK_EMBEDDED_C})
add_dependencies(soccerkid embed_data)
target_link_libraries(soccerkid PRIVATE skcore SDL3::SDL3)
if(WIN32)                                               # a window, no console; SDL3.dll next to the program
  set_target_properties(soccerkid PROPERTIES WIN32_EXECUTABLE TRUE)
  add_custom_command(TARGET soccerkid POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_RUNTIME_DLLS:soccerkid> $<TARGET_FILE_DIR:soccerkid>
    COMMAND_EXPAND_LISTS)
endif()

if(APPLE)                                               # cmake --build build --target app: Soccer Kid.app
  add_executable(skicon tools/skicon.c)
  target_link_libraries(skicon PRIVATE skcore)
  add_custom_target(app
    COMMAND ${CMAKE_COMMAND} -DBIN=$<TARGET_FILE:soccerkid> -DSKICON=$<TARGET_FILE:skicon>
            -DDATA=${CMAKE_SOURCE_DIR}/data/soccerkid.dat -DPLIST=${CMAKE_SOURCE_DIR}/packaging/Info.plist.in
            -DVERSION=${SK_VERSION} -DOUT=${CMAKE_BINARY_DIR} -P ${CMAKE_SOURCE_DIR}/packaging/make_app.cmake
    DEPENDS soccerkid skicon
    COMMENT "Packing Soccer Kid.app"
    VERBATIM)
endif()

enable_testing()
add_subdirectory(tests)
