cmake_minimum_required(VERSION 3.29)
project(bfWindow LANGUAGES CXX ASM_MASM)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS OFF)

# --- Shared library ---
add_library(bf.Window SHARED
    ../lib/Window/CPP/Window.cpp
    ../WorkDirectory/ASMWindow.asm
    ../WorkDirectory/ASMWindowHelper.asm
    ../WorkDirectory/ASMColor.asm
)

# --- Module interface ---
target_sources(bf.Window
    PUBLIC
        FILE_SET CXX_MODULES
        BASE_DIRS ../lib/Window/IXX
        FILES ../lib/Window/IXX/Window.ixx
)

# --- Export symbols ---
target_compile_definitions(bf.Window PRIVATE BF_WINDOW_EXPORTS)

# --- System libs ---
target_link_libraries(bf.Window PRIVATE user32 gdi32 kernel32)

# --- Output directories ---
set_target_properties(bf.Window PROPERTIES
    OUTPUT_NAME "bf-Window"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

# --- Include module metadata for dependents ---
target_include_directories(bf.Window PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/bf.Window.dir>
)
