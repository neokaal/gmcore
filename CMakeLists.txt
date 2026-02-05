cmake_minimum_required(VERSION 3.16)
project(gfxlc VERSION 0.1 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Prefer shared SDL build
set(SDL_SHARED ON  CACHE BOOL "" FORCE)
set(SDL_STATIC OFF CACHE BOOL "" FORCE)
set(SDL_TEST OFF   CACHE BOOL "" FORCE)

include(FetchContent)
FetchContent_Declare(
    SDL3
    URL https://github.com/libsdl-org/SDL/releases/download/release-3.4.0/SDL3-3.4.0.zip
)
FetchContent_MakeAvailable(SDL3)

# Fetch Lua (5.5.0) and build a static library from its sources
FetchContent_Declare(
    lua
    URL https://www.lua.org/ftp/lua-5.5.0.tar.gz
)
FetchContent_MakeAvailable(lua)

# Lua: create a static library from the Lua sources (exclude lua/luac programs)
file(GLOB LUA_SRC CONFIGURE_DEPENDS "${lua_SOURCE_DIR}/src/*.c")
list(REMOVE_ITEM LUA_SRC
    "${lua_SOURCE_DIR}/src/lua.c"
    "${lua_SOURCE_DIR}/src/luac.c"
)
add_library(lua STATIC ${LUA_SRC})
target_include_directories(lua PUBLIC "${lua_SOURCE_DIR}/src")
# On Linux, link libm (math)
if (UNIX AND NOT APPLE)
    target_link_libraries(lua PRIVATE m)
endif()
add_library(Lua::Lua ALIAS lua)

# Sources
set(SRC_FILES
    src/main.c
)

add_executable(gfxlc ${SRC_FILES})

# Includes
target_include_directories(gfxlc PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ext/include)

# Compiler warnings
if (MSVC)
    target_compile_options(gfxlc PRIVATE /W4)
else()
    target_compile_options(gfxlc PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Link SDL and Lua
target_link_libraries(gfxlc PRIVATE SDL3::SDL3 Lua::Lua)

# Link OpenGL
find_package(OpenGL REQUIRED)
if (TARGET OpenGL::GL)
    target_link_libraries(gfxlc PRIVATE OpenGL::GL)
else()
    if (WIN32)
        target_link_libraries(gfxlc PRIVATE opengl32)
    endif()
endif()

# On Linux, link dl (used by glad for runtime loading)
if (UNIX AND NOT APPLE)
    find_library(DL_LIBRARY dl)
    if (DL_LIBRARY)
        target_link_libraries(gfxlc PRIVATE ${DL_LIBRARY})
    endif()
endif()

# On Windows, copy the SDL3 runtime DLL next to the executable after build so
# the binary can be run directly from the build tree without installing.
if (WIN32 AND TARGET SDL3::SDL3)
    add_custom_command(TARGET gfxlc POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:SDL3::SDL3>
            $<TARGET_FILE_DIR:gfxlc>
    )
endif()

# Put binaries in a top-level bin/ like the Makefile does
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Install rule (optional)
install(TARGETS gfxlc RUNTIME DESTINATION bin)
# On Windows, install the SDL3 runtime DLL into the same bin directory so
# the installed package can be run directly from the install tree.
if (WIN32 AND TARGET SDL3::SDL3)
    # Use install(FILES ...) with a generator expression to reference the
    # built SDL3 DLL directly. The OPTIONAL keyword avoids errors when the
    # file isn't present during configure/unavailable configurations.
    install(FILES $<TARGET_FILE:SDL3::SDL3> DESTINATION bin OPTIONAL)
endif()
