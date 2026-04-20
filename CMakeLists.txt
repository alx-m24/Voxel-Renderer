cmake_minimum_required(VERSION 3.21)
project(Voxel-Renderer LANGUAGES CXX)

# ---- Language ----
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---- Main output ----
add_library(${PROJECT_NAME} STATIC
    src/VoxelRenderer.cpp
)

# ---- FetchContent ----
include(FetchContent)

# LexviEngine
FetchContent_Declare(
    LexviEngine
    GIT_REPOSITORY https://github.com/alx-m24/LexviEngine
    GIT_TAG old-engine
)
FetchContent_MakeAvailable(LexviEngine)

target_link_libraries(${PROJECT_NAME} 
    PRIVATE 
        LexviEngine
)

target_include_directories(${PROJECT_NAME}
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/Headers
    PRIVATE
        ${LexviEngine_SOURDE_DIR}
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
