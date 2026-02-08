cmake_minimum_required(VERSION 3.16)
project(gfxlc VERSION 0.1 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Sources
set(SRC_FILES
    src/gfxlc_util.c
    src/gfxlc_lua.c
    src/main.c
)

set(ASSETS_DIR "${CMAKE_SOURCE_DIR}/assets")
set(FONTS_DIR "${ASSETS_DIR}/fonts")
set(SOURCE_CODE_PRO_FONT "${FONTS_DIR}/source_code_pro/SourceCodePro-Regular.ttf")
set(FONTS_DEST_DIR "${CMAKE_CURRENT_BINARY_DIR}")


add_executable(${PROJECT_NAME} ${SRC_FILES})

# Link SDL and Lua
find_package(Lua REQUIRED)
target_include_directories(${PROJECT_NAME} PRIVATE ${LUA_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE ${LUA_LIBRARIES})

find_package(SDL3 CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3)

find_package(SDL3_ttf CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE SDL3_ttf::SDL3_ttf)

# Copy source code pro font file to binary directory
configure_file(
    "${SOURCE_CODE_PRO_FONT}"
    "${FONTS_DEST_DIR}"
    COPYONLY
)
