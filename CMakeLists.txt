cmake_minimum_required(VERSION 3.21)
project(AE3DFlag VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# After Effects SDK path — set via CMake variable or environment variable
# ---------------------------------------------------------------------------
if(NOT DEFINED AE_SDK_DIR)
    if(DEFINED ENV{AE_SDK_DIR})
        set(AE_SDK_DIR "$ENV{AE_SDK_DIR}" CACHE PATH "Path to the Adobe AE SDK root")
    else()
        message(FATAL_ERROR
            "AE_SDK_DIR not set. Pass it to CMake:\n"
            "  cmake -DAE_SDK_DIR=/path/to/AESDK ..")
    endif()
endif()

# SDK header directories (adjust if your SDK layout differs)
set(AE_SDK_HEADERS
    "${AE_SDK_DIR}/Headers"
    "${AE_SDK_DIR}/Headers/SP"
    "${AE_SDK_DIR}/Resources"
    "${AE_SDK_DIR}/Util"
)

# ---------------------------------------------------------------------------
# Source files
# ---------------------------------------------------------------------------
set(PLUGIN_SOURCES
    src/AE3DFlag.cpp
    src/FlagRenderer.cpp
    # Required SDK utility files
    "${AE_SDK_DIR}/Util/AEGP_SuiteHandler.cpp"
    "${AE_SDK_DIR}/Util/AEFX_SuiteHelper.c"
)

set(PLUGIN_HEADERS
    src/AE3DFlag.h
    src/FlagRenderer.h
)

# ---------------------------------------------------------------------------
# Build target — shared library renamed to .aex bundle on macOS
# ---------------------------------------------------------------------------
add_library(AE3DFlag MODULE ${PLUGIN_SOURCES} ${PLUGIN_HEADERS})

target_include_directories(AE3DFlag PRIVATE
    ${AE_SDK_HEADERS}
    src
)

# ---------------------------------------------------------------------------
# macOS-specific bundle settings
# ---------------------------------------------------------------------------
if(APPLE)
    set_target_properties(AE3DFlag PROPERTIES
        BUNDLE                      TRUE
        BUNDLE_EXTENSION            "aex"
        MACOSX_BUNDLE_INFO_PLIST    "${CMAKE_CURRENT_SOURCE_DIR}/res/Info.plist"
        OUTPUT_NAME                 "AE3DFlag"
        PREFIX                      ""
        SUFFIX                      ".aex"
    )

    # Build universal binary (Intel + Apple Silicon)
    set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "" FORCE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "" FORCE)

    target_compile_options(AE3DFlag PRIVATE
        -Wall -Wextra -Wno-unused-parameter
        -fvisibility=hidden                # hide all symbols by default
        -ffast-math
        -O2
    )

    # Export only EffectMain
    target_link_options(AE3DFlag PRIVATE
        -exported_symbol _EffectMain
    )

    # PiPL resource: find Rez via xcrun (modern macOS) or fallback paths
    execute_process(
        COMMAND xcrun --find Rez
        OUTPUT_VARIABLE REZ_TOOL
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET)

    if(NOT REZ_TOOL)
        find_program(REZ_TOOL Rez
            PATHS /usr/bin /Developer/Tools
                  "/Applications/Xcode.app/Contents/Developer/usr/bin"
                  "${AE_SDK_DIR}/Resources"
            NO_DEFAULT_PATH)
    endif()

    if(REZ_TOOL)
        message(STATUS "Rez found: ${REZ_TOOL}")
        set(PIPL_R   "${CMAKE_CURRENT_SOURCE_DIR}/res/AE3DFlag.r")
        set(PIPL_RSC "${CMAKE_CURRENT_BINARY_DIR}/AE3DFlag.rsrc")

        # Build include path list for Rez
        set(REZ_INCLUDE_FLAGS "")
        foreach(inc ${AE_SDK_HEADERS})
            list(APPEND REZ_INCLUDE_FLAGS -i "${inc}")
        endforeach()

        add_custom_command(
            OUTPUT  ${PIPL_RSC}
            COMMAND ${REZ_TOOL}
                    ${REZ_INCLUDE_FLAGS}
                    -DAE_OS_MAC=1 -DAE_PROC_ARM64=1
                    -o ${PIPL_RSC}
                    ${PIPL_R}
            DEPENDS ${PIPL_R}
            COMMENT "Compiling PiPL resource with ${REZ_TOOL}"
        )
        add_custom_target(PiPL_Resource DEPENDS ${PIPL_RSC})
        add_dependencies(AE3DFlag PiPL_Resource)

        # Copy .rsrc into the bundle Resources folder after linking
        add_custom_command(TARGET AE3DFlag POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory
                    "$<TARGET_BUNDLE_CONTENT_DIR:AE3DFlag>/Resources"
            COMMAND ${CMAKE_COMMAND} -E copy
                    ${PIPL_RSC}
                    "$<TARGET_BUNDLE_CONTENT_DIR:AE3DFlag>/Resources/AE3DFlag.rsrc"
            COMMENT "Embedding PiPL resource in bundle"
        )
    else()
        message(WARNING "Rez not found — PiPL resource will NOT be compiled.")
    endif()
endif()

# ---------------------------------------------------------------------------
# Windows (reference, not actively maintained for this project)
# ---------------------------------------------------------------------------
if(WIN32)
    set_target_properties(AE3DFlag PROPERTIES
        OUTPUT_NAME "AE3DFlag"
        SUFFIX      ".aex"
        PREFIX      ""
    )
    target_compile_options(AE3DFlag PRIVATE /W3 /O2)
    target_compile_definitions(AE3DFlag PRIVATE WIN32 _WINDOWS)
endif()

# ---------------------------------------------------------------------------
# Install — copy to AE plug-ins folder
# ---------------------------------------------------------------------------
if(DEFINED AE_PLUGIN_DIR)
    install(TARGETS AE3DFlag
            BUNDLE  DESTINATION "${AE_PLUGIN_DIR}"
            LIBRARY DESTINATION "${AE_PLUGIN_DIR}")
endif()

message(STATUS "AE3DFlag — SDK: ${AE_SDK_DIR}")
message(STATUS "AE3DFlag — Architectures: ${CMAKE_OSX_ARCHITECTURES}")
