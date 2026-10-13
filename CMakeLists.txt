cmake_minimum_required(VERSION 3.24)
project(soccerkid C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()
add_compile_options(-Wall -Wextra -Werror -Wshadow -Wconversion -Wno-sign-conversion)
add_compile_definitions(_POSIX_C_SOURCE=200809L _DARWIN_C_SOURCE)

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
  src/formats/sections.c
  src/formats/bobs.c
  src/formats/screens.c
  src/gfx/framebuffer.c
  src/gfx/palette.c
  src/gfx/present.c
  src/gfx/png.c
  src/gfx/sprites.c
  src/app/args.c
  src/app/scene.c
  src/app/play.c
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
  src/game/attr_country_london.c)
target_include_directories(skcore PUBLIC src)
target_link_libraries(skcore PUBLIC ZLIB::ZLIB SDL3::SDL3)

add_executable(soccerkid src/app/main.c)
target_link_libraries(soccerkid PRIVATE skcore SDL3::SDL3)

enable_testing()
add_subdirectory(tests)
