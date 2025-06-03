cmake_minimum_required(VERSION 3.28)
project(Plotter LANGUAGES CXX VERSION 1.0)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_BUILD_TYPE Debug)

include(FetchContent)

# Fetch SFML
FetchContent_Declare(sfml
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 3.0.0
    GIT_SHALLOW ON
    SYSTEM
)

# Disable building audio & network for SFML
set(SFML_BUILD_AUDIO    OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_NETWORK  OFF CACHE BOOL "" FORCE)

# Disable examples & docs for SFML
set(SFML_BUILD_EXAMPLES    OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_DOC         OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(sfml)

# Fetch imgui
FetchContent_Declare(imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG master
    GIT_SHALLOW ON
    SYSTEM
)
FetchContent_MakeAvailable(imgui)

# Fetch imgui-sfml
FetchContent_Declare(imgui-sfml
    GIT_REPOSITORY https://github.com/eliasdaler/imgui-sfml.git
    GIT_TAG master
    GIT_SHALLOW ON
    SYSTEM
)

# Disable examples for ImGui-SFML
set(IMGUI_BUILD_EXAMPLES        OFF CACHE BOOL "" FORCE)
set(IMGUI_SFML_BUILD_EXAMPLES   OFF CACHE BOOL "" FORCE)

set(IMGUI_DIR ${imgui_SOURCE_DIR})
set(IMGUI_SFML_FIND_SFML OFF)

FetchContent_MakeAvailable(imgui-sfml)

# Fetch GiNaC
FetchContent_Declare(ginac
    GIT_REPOSITORY git://www.ginac.de/ginac.git
    GIT_TAG master
    GIT_SHALLOW ON
    SYSTEM
)

# Disable examples & docs for GiNaC
set(GINAC_BUILD_EXAMPLES    OFF CACHE BOOL "" FORCE)
set(GINAC_BUILD_DOC         OFF CACHE BOOL "" FORCE)
set(GINAC_BUILD_TESTS       OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(ginac)

# Set sources for exprlib
set(EXPRLIB_SOURCES
    src/ExprLib.cpp
    src/Expression.cpp
    src/ExpressionCalculator.cpp
)

add_library(exprlib_static STATIC ${EXPRLIB_SOURCES})
target_compile_features(exprlib_static PRIVATE cxx_std_17)

target_link_libraries(exprlib_static
    ginac::ginac
)

# Create main executable
add_executable(main 
    src/main.cpp
    src/Graph.cpp
    src/Plotter.cpp
    src/Roboto_font.cpp
    src/RobotoMono_font.cpp)
target_compile_features(main PRIVATE cxx_std_17)

# Link libraries
target_link_libraries(main
    PRIVATE
        SFML::Graphics
        SFML::System
        ImGui-SFML::ImGui-SFML
        exprlib_static
)

# Create executable for tests
add_executable(tests
    tests/func_tests.cpp
    src/Expression.cpp
    src/ExpressionCalculator.cpp)
target_compile_features(tests PRIVATE cxx_std_17)

target_link_libraries(tests
    PRIVATE
        ginac::ginac
)

target_compile_options(main PRIVATE "$<$<CONFIG:Debug>:-ggdb>")
target_compile_options(tests PRIVATE "$<$<CONFIG:Debug>:-ggdb>")
