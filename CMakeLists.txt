cmake_minimum_required(VERSION 3.22)

project(LuaAutoCompleteQt6
        VERSION 1.0.0
        DESCRIPTION "Qt6 Lua Editor with AutoComplete"
        LANGUAGES CXX)

# C++ Standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic -Wconversion)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-g -O0 -fsanitize=address)
        add_link_options(-fsanitize=address)
    else()
        add_compile_options(-O3 -DNDEBUG)
    endif()
endif()

# Qt6 packages
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui)

# ---- Lua dependency ----
find_package(PkgConfig QUIET)

if(PkgConfig_FOUND)
    # Versuche verschiedene pkg-config Namen (Lua 5.4, 5.3, oder generisch)
    pkg_check_modules(LUA QUIET lua5.4)
    if(NOT LUA_FOUND)
        pkg_check_modules(LUA QUIET lua5.3)
    endif()
    if(NOT LUA_FOUND)
        pkg_check_modules(LUA QUIET lua)
    endif()
endif()

# Fallback auf CMake FindLua (funktioniert bei vielen Distros)
if(NOT LUA_FOUND)
    find_package(Lua REQUIRED)
    set(LUA_LIBRARIES ${LUA_LIBRARIES})
    set(LUA_INCLUDE_DIRS ${LUA_INCLUDE_DIR})
endif()

# ---- Enable Qt features ----
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# ---- Sources ----
set(SOURCES
        src/main.cpp
        src/MainWindow.cpp
        src/LuaEditor.cpp
        src/LuaParser.cpp
        src/AutoCompleter.cpp
        src/LuaHighlighter.cpp
)

set(HEADERS
        src/MainWindow.h
        src/LuaEditor.h
        src/LuaParser.h
        src/AutoCompleter.h
        src/LuaHighlighter.h
)

# ---- Executable ----
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# ---- Linker & Includes ----
target_link_libraries(${PROJECT_NAME}
        Qt6::Core
        Qt6::Widgets
        Qt6::Gui
        ${LUA_LIBRARIES}
)

target_include_directories(${PROJECT_NAME}
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${LUA_INCLUDE_DIRS}
)

# Optional: nur setzen, wenn vorhanden
if(LUA_CFLAGS_OTHER)
    target_compile_definitions(${PROJECT_NAME} PRIVATE ${LUA_CFLAGS_OTHER})
endif()

# ---- Install ----
install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib/static
)

install(DIRECTORY resources/
        DESTINATION share/${PROJECT_NAME}
        FILES_MATCHING
        PATTERN "*.png"
        PATTERN "*.qss"
        PATTERN "*.ts"
)

# ---- Tests ----
enable_testing()
add_subdirectory(tests)

# ---- Info ----
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Qt6 version: ${Qt6_VERSION}")
if(LUA_FOUND)
    message(STATUS "Lua libraries: ${LUA_LIBRARIES}")
    message(STATUS "Lua include dirs: ${LUA_INCLUDE_DIRS}")
endif()
