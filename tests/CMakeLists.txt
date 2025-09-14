# Tests CMakeLists.txt
cmake_minimum_required(VERSION 3.22)

# Find Qt6 Test module
find_package(Qt6 REQUIRED COMPONENTS Test)

# Include parent directory for headers
include_directories(${CMAKE_SOURCE_DIR}/src)

# Test sources
set(TEST_SOURCES
    test_parser.cpp
    test_autocompleter.cpp
)

# Test data
configure_file(test_data/sample.lua test_data/sample.lua COPYONLY)

# Create test executables
foreach(test_source ${TEST_SOURCES})
    get_filename_component(test_name ${test_source} NAME_WE)
    
    add_executable(${test_name}
        ${test_source}
        ${CMAKE_SOURCE_DIR}/src/LuaParser.cpp
        ${CMAKE_SOURCE_DIR}/src/AutoCompleter.cpp
        ${CMAKE_SOURCE_DIR}/src/LuaHighlighter.cpp
    )
    
    target_link_libraries(${test_name}
        Qt6::Test
        Qt6::Core
        Qt6::Widgets
        Qt6::Gui
        ${LUA_LIBRARIES}
    )
    
    target_include_directories(${test_name} PRIVATE ${LUA_INCLUDE_DIRS})
    target_compile_definitions(${test_name} PRIVATE ${LUA_CFLAGS_OTHER})
    
    # Add test
    add_test(NAME ${test_name} COMMAND ${test_name})
    
    # Set test properties
    set_tests_properties(${test_name} PROPERTIES
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        TIMEOUT 30
    )
endforeach()

message(STATUS "Configured ${CMAKE_CURRENT_SOURCE_DIR} tests")
