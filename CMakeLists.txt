cmake_minimum_required(VERSION 3.24)
project(soccerkid C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()
add_compile_options(-Wall -Wextra -Werror -Wshadow -Wconversion -Wno-sign-conversion)

find_package(SDL3 CONFIG REQUIRED)
find_package(ZLIB REQUIRED)

add_library(skcore STATIC
  src/core/err.c)
target_include_directories(skcore PUBLIC src)
target_link_libraries(skcore PUBLIC ZLIB::ZLIB)

add_executable(soccerkid src/app/main.c)
target_link_libraries(soccerkid PRIVATE skcore SDL3::SDL3)

enable_testing()
add_subdirectory(tests)
