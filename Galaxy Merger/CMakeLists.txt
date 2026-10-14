cmake_minimum_required(VERSION 3.24)
project(GalaxyMerger LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include(FetchContent)
include(CTest)

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
    GIT_SHALLOW TRUE
)
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.91.5
    GIT_SHALLOW TRUE
)
FetchContent_Declare(
    doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
    GIT_TAG v2.4.11
    GIT_SHALLOW TRUE
)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
set(DOCTEST_NO_INSTALL ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(glfw doctest)

FetchContent_GetProperties(imgui)
if(NOT imgui_POPULATED)
    FetchContent_Populate(imgui)
endif()

find_package(OpenMP)

add_library(physics STATIC
    src/core/state.cpp
    src/visualization/optical.cpp
    src/visualization/tracers.cpp
    src/forces/forces.cpp
    src/forces/hydro.cpp
    src/integrators/integrators.cpp
    src/simulation/scene.cpp
    src/simulation/galaxy.cpp
    src/simulation/validation.cpp
    src/io/sphere.cpp
    src/io/trajectory.cpp
    src/io/gltf.cpp
)
target_include_directories(physics PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_features(physics PUBLIC cxx_std_20)
if(OpenMP_CXX_FOUND)
    target_link_libraries(physics PUBLIC OpenMP::OpenMP_CXX)
endif()
if(MSVC)
    target_compile_options(physics PRIVATE /W4 /permissive-)
else()
    target_compile_options(physics PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(physics_tests
    tests/test_main.cpp
    tests/test_populations.cpp
    tests/test_optical.cpp
    tests/test_tracers.cpp
    tests/test_state.cpp
    tests/test_nbody.cpp
    tests/test_scene_io.cpp
    tests/test_galaxy.cpp
    tests/test_hydro.cpp
)
target_link_libraries(physics_tests PRIVATE physics doctest::doctest)
target_compile_definitions(physics_tests PRIVATE PHYSICS_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
if(MSVC)
    target_compile_options(physics_tests PRIVATE /W4 /permissive-)
else()
    target_compile_options(physics_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()
add_test(NAME physics_tests COMMAND physics_tests)

add_executable(physics_run apps/run/main.cpp)
add_executable(physics_validate apps/validate/main.cpp)
add_executable(physics_audit apps/audit/main.cpp)
target_link_libraries(physics_audit PRIVATE physics)
add_executable(physics_trace apps/trace/main.cpp)
target_link_libraries(physics_trace PRIVATE physics)
target_link_libraries(physics_validate PRIVATE physics)
foreach(audit_target physics_validate physics_audit physics_trace)
    if(MSVC)
        target_compile_options(${audit_target} PRIVATE /W4 /permissive-)
    else()
        target_compile_options(${audit_target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endforeach()
target_link_libraries(physics_run PRIVATE physics)
if(MSVC)
    target_compile_options(physics_run PRIVATE /W4 /permissive-)
else()
    target_compile_options(physics_run PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_library(imgui_lib STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui_lib PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
target_link_libraries(imgui_lib PUBLIC glfw)
target_compile_features(imgui_lib PRIVATE cxx_std_11)
if(MSVC)
    target_compile_options(imgui_lib PRIVATE /w)
else()
    target_compile_options(imgui_lib PRIVATE -w)
endif()

add_executable(physics_view
    apps/viewer/main.cpp
    apps/viewer/gl_loader.cpp
    apps/viewer/frame_export.cpp
)
target_include_directories(physics_view PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/apps/viewer ${CMAKE_CURRENT_SOURCE_DIR}/third_party)
target_link_libraries(physics_view PRIVATE physics imgui_lib glfw)
if(WIN32)
    target_link_libraries(physics_view PRIVATE opengl32 ole32 shell32)
    set_target_properties(physics_view PROPERTIES WIN32_EXECUTABLE TRUE)
    target_link_options(physics_view PRIVATE "/ENTRY:mainCRTStartup")
elseif(APPLE)
    find_library(OPENGL_LIBRARY OpenGL REQUIRED)
    target_link_libraries(physics_view PRIVATE ${OPENGL_LIBRARY})
else()
    target_link_libraries(physics_view PRIVATE GL)
endif()
if(MSVC)
    target_compile_options(physics_view PRIVATE /W4 /permissive-)
else()
    target_compile_options(physics_view PRIVATE -Wall -Wextra -Wpedantic)
endif()
add_custom_command(TARGET physics_view POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/examples"
        "$<TARGET_FILE_DIR:physics_view>/examples"
)
