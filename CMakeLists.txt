cmake_minimum_required(VERSION 3.21)
project(Voxel-Renderer LANGUAGES CXX)

# ---- Language ----
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---- Executable ----
add_library(${PROJECT_NAME}
    src/VoxelRenderer.cpp
)

# ---- FetchContent ----
include(FetchContent)

# GLFW
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

# GLFW
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
)
FetchContent_MakeAvailable(glfw)

# GLM
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.3
)
FetchContent_MakeAvailable(glm)

# GLAD
FetchContent_Declare(
    glad
    GIT_REPOSITORY https://github.com/Dav1dde/glad.git
    GIT_TAG v0.1.36
)
FetchContent_MakeAvailable(glad)

# STB
FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
)
FetchContent_MakeAvailable(stb)

# ASSIMP
FetchContent_Declare(assimp
    GIT_REPOSITORY https://github.com/assimp/assimp.git
    GIT_TAG v5.4.3
)
FetchContent_MakeAvailable(assimp)
set_target_properties(assimp PROPERTIES COMPILE_FLAGS "-Wno-error")
target_compile_options(assimp PRIVATE
    -Wno-error
    -Wno-unknown-pragmas
    -Wno-implicit-fallthrough
    -Wno-deprecated-copy
    -Wno-ignored-qualifiers
)

# IMGUI & IMPLOT
FetchContent_Declare(
    imgui_fetch
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.92.7
)
FetchContent_MakeAvailable(imgui_fetch)

FetchContent_Declare(
    implot_fetch
    GIT_REPOSITORY https://github.com/epezent/implot.git
    GIT_TAG master
)
FetchContent_MakeAvailable(implot_fetch)

add_library(imgui STATIC
    ${imgui_fetch_SOURCE_DIR}/imgui.cpp
    ${imgui_fetch_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_fetch_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_fetch_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_fetch_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_fetch_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC
    ${imgui_fetch_SOURCE_DIR}
    ${imgui_fetch_SOURCE_DIR}/backends
    ${glfw_SOURCE_DIR}/include
    ${glad_SOURCE_DIR}/include
)
target_link_libraries(imgui PUBLIC glfw glad)

add_library(implot STATIC
    ${implot_fetch_SOURCE_DIR}/implot.cpp
    ${implot_fetch_SOURCE_DIR}/implot_items.cpp
)
target_include_directories(implot PUBLIC
    ${implot_fetch_SOURCE_DIR}
    ${imgui_fetch_SOURCE_DIR}
)
target_link_libraries(implot PUBLIC imgui)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        assimp
        glfw
        glm
        glad
        imgui
        implot
)

target_include_directories(${PROJECT_NAME}
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Headers
        ${implot_fetch_SOURCE_DIR}
        ${implot_fetch_SOURCE_DIR}/backends
        ${imgui_fetch_SOURCE_DIR}
        ${glm_SOURCE_DIR}
        ${glfw_SOURCE_DIR}/include
        ${stb_SOURCE_DIR}
        ${glad_SOURCE_DIR}/include
        ${assimp_SOURCE_DIR}/include
)

# Post build - Copy Models and shaders
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Models"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/src/Models"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Shaders"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/src/Shaders"
)

# ---- MinGW runtime DLL copy (Windows only) ----
if(WIN32 AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    foreach(dll
        libstdc++-6.dll
        libgcc_s_seh-1.dll
        libwinpthread-1.dll
    )
        execute_process(
            COMMAND ${CMAKE_CXX_COMPILER} -print-file-name=${dll}
            OUTPUT_VARIABLE DLL_PATH
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )

        if(EXISTS ${DLL_PATH})
            add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${DLL_PATH}"
                $<TARGET_FILE_DIR:${PROJECT_NAME}>
            )
        endif()
    endforeach()
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(${PROJECT_NAME} PRIVATE -static-libgcc)
endif()
