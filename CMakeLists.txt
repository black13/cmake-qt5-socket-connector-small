cmake_minimum_required(VERSION 3.16)
project(NodeGraph VERSION 1.0.0 LANGUAGES CXX)

# ─────────────────────────────────────
# 1. C++ standard and project settings
# ─────────────────────────────────────
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable position independent code for better optimization
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# ─────────────────────────────────────
# 2. Platform-specific settings - Enhanced Win32 Optimization
# ─────────────────────────────────────
if(WIN32)
    set(CMAKE_SYSTEM_VERSION 10.0)
    add_compile_definitions(WIN32_LEAN_AND_MEAN NOMINMAX)

    # Enable parallel compilation for MSVC
    add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/MP>)

    # Windows-specific optimizations
    add_compile_options(
        $<$<CONFIG:Release>:/O2>  # Optimize for speed
        $<$<CONFIG:Release>:/Ob2> # Inline function expansion
        $<$<CONFIG:Release>:/Oi>   # Enable intrinsic functions
        $<$<CONFIG:Release>:/Ot>   # Favor speed over size
        $<$<CONFIG:Release>:/Oy>   # Omit frame pointers
        $<$<CONFIG:Release>:/GL>   # Enable link-time code generation
        $<$<CONFIG:Release>:/Gy>   # Separate functions for linker
        $<$<CONFIG:Release>:/arch:AVX2> # Use AVX2 instructions
    )

    # Linker optimizations for Release builds
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG /INCREMENTAL:NO")
    set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /LTCG /INCREMENTAL:NO")
    set(CMAKE_MODULE_LINKER_FLAGS_RELEASE "${CMAKE_MODULE_LINKER_FLAGS_RELEASE} /LTCG /INCREMENTAL:NO")

    # Precompiled headers for MSVC
    set(CMAKE_PCH_INSTANTIATE_TEMPLATES ON)
    set(CMAKE_MSVC_PCH ON)

    # Additional Win32-specific optimizations
    add_compile_definitions(
        _CRT_SECURE_NO_WARNINGS
        _SCL_SECURE_NO_WARNINGS
        _WIN32_WINNT=0x0A00  # Windows 10
        _ENABLE_EXTENDED_ALIGNED_STORAGE
    )

    # Enable whole program optimization for Release builds
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_compile_options(/Oi /Ot /Oy /GL)
        set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG")
    endif()
endif()

# ─────────────────────────────────────
# 3. Build type configuration
# ─────────────────────────────────────
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# Optimization flags for Release builds
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_options($<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-O3 -DNDEBUG>)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

# ─────────────────────────────────────
# 3a. Code Coverage Configuration
# ─────────────────────────────────────
option(ENABLE_COVERAGE "Enable code coverage instrumentation" OFF)

if(ENABLE_COVERAGE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        # Clang/LLVM coverage
        add_compile_options(-fprofile-instr-generate -fcoverage-mapping)
        add_link_options(-fprofile-instr-generate)
        message(STATUS "Code coverage enabled (Clang): Use llvm-cov for analysis")
        message(STATUS "  Set LLVM_PROFILE_FILE environment variable before running")
        message(STATUS "  Example: export LLVM_PROFILE_FILE=\"nodegraph-%p.profraw\"")
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        # GCC coverage (gcov)
        add_compile_options(--coverage)
        add_link_options(--coverage)
        message(STATUS "Code coverage enabled (GCC): Use gcov/lcov for analysis")
    else()
        message(WARNING "Code coverage requested but not supported for: ${CMAKE_CXX_COMPILER_ID}")
    endif()
endif()

# ─────────────────────────────────────
# 4. Qt configuration with optimized search
# ─────────────────────────────────────
# Use Qt's built-in CMake configuration
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Set Qt paths based on build type
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(QT5_PATH "E:/Qt/5.15.16/Debug_x64")
else()
    set(QT5_PATH "E:/Qt/5.15.16/Release_x64")
endif()

# Preferred Qt installation paths (Windows)
if(WIN32)
    list(APPEND CMAKE_PREFIX_PATH "${QT5_PATH}/lib/cmake")
    set(QT_QMAKE_EXECUTABLE "${QT5_PATH}/bin/qmake.exe")
endif()

# Find Qt components with REQUIRED for early failure
find_package(Qt5 REQUIRED COMPONENTS Core Widgets Gui Qml)

# ─────────────────────────────────────
# 5. libxml2 configuration with caching optimization
# ─────────────────────────────────────
include(FetchContent)

# Use a persistent cache directory
set(FETCHCONTENT_BASE_DIR "${CMAKE_BINARY_DIR}/_deps" CACHE PATH "FetchContent base directory")

# Configure libxml2 build options before making it available
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build static libraries" FORCE)
set(LIBXML2_WITH_ICONV OFF CACHE BOOL "Disable iconv support" FORCE)
set(LIBXML2_WITH_LZMA OFF CACHE BOOL "Disable LZMA support" FORCE)
set(LIBXML2_WITH_ZLIB OFF CACHE BOOL "Disable zlib support" FORCE)
set(LIBXML2_WITH_PYTHON OFF CACHE BOOL "Disable Python bindings" FORCE)
set(LIBXML2_WITH_TESTS OFF CACHE BOOL "Disable tests" FORCE)
set(LIBXML2_WITH_PROGRAMS OFF CACHE BOOL "Disable programs" FORCE)
set(LIBXML2_WITH_HTTP OFF CACHE BOOL "Disable HTTP support" FORCE)
set(LIBXML2_WITH_FTP OFF CACHE BOOL "Disable FTP support" FORCE)

# Use a pre-downloaded libxml2 source for faster builds
# Only enable disconnected mode if source already exists
if(EXISTS "${CMAKE_BINARY_DIR}/_deps/libxml2-src/CMakeLists.txt")
    set(FETCHCONTENT_FULLY_DISCONNECTED ON)
    set(FETCHCONTENT_UPDATES_DISCONNECTED ON)
endif()

FetchContent_Declare(
    libxml2
    GIT_REPOSITORY https://gitlab.gnome.org/GNOME/libxml2.git
    GIT_TAG        v2.12.5
    GIT_SHALLOW    TRUE
    SOURCE_DIR     "${CMAKE_BINARY_DIR}/_deps/libxml2-src"
)

FetchContent_MakeAvailable(libxml2)

# Create alias target for consistency (check if target exists)
if(TARGET libxml2 AND NOT TARGET LibXml2::LibXml2)
    add_library(LibXml2::LibXml2 ALIAS libxml2)
elseif(TARGET LibXml2::LibXml2)
    # Already aliased, do nothing
elseif(TARGET xml2)
    # Alternative target name
    add_library(LibXml2::LibXml2 ALIAS xml2)
endif()

add_definitions(-DUSE_LIBXML2)

# ─────────────────────────────────────
# 6. Source file organization
# ─────────────────────────────────────
# Core library sources
set(CORE_SOURCES
    # Node/Edge/Socket architecture
    node.h
    node.cpp
    socket.h
    socket.cpp
    edge.h
    edge.cpp
    ghost_edge.h
    ghost_edge.cpp
    # Graph architecture
    qgraph.h
    qgraph.cpp
    graph_controller.h
    graph_controller.cpp
    # Factory and templates
    node_templates.h
    node_templates.cpp
    graph_factory.h
    graph_factory.cpp
    # UI components
    window.h
    window.cpp
    view.h
    view.cpp
    scene.h
    scene.cpp
    node_palette_widget.h
    node_palette_widget.cpp
    # JavaScript integration
    javascript_engine.h
    javascript_engine.cpp
    # Observer pattern
    graph_observer.h
    graph_observer.cpp
    xml_autosave_observer.h
    xml_autosave_observer.cpp
)

# Main application sources
set(MAIN_SOURCES
    main.cpp
    icons.qrc
)

# ─────────────────────────────────────
# 7. Core library target with modern CMake
# ─────────────────────────────────────
add_library(NodeGraphCore STATIC ${CORE_SOURCES})

# Include directories
target_include_directories(NodeGraphCore
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
)

# Compile definitions
# Remove QT_NO_KEYWORDS to allow signals/slots/emit
#target_compile_definitions(NodeGraphCore
#    PRIVATE
#        QT_NO_KEYWORDS
#)

# Link libraries
target_link_libraries(NodeGraphCore
    PRIVATE
        Qt5::Core
        Qt5::Widgets
        Qt5::Gui
        Qt5::Qml
        LibXml2::LibXml2
)

# Compiler-specific optimizations
if(MSVC)
    target_compile_options(NodeGraphCore
        PRIVATE
            /W4
            /wd4100  # Disable "unreferenced parameter" warnings
            /wd4127  # Disable "conditional expression is constant"
            /wd4251  # Disable "needs DLL interface" warnings
            $<$<CONFIG:Release>:/GL>  # Enable link-time code generation
            $<$<CONFIG:Release>:/arch:AVX2>  # Use AVX2 instructions
    )
else()
    target_compile_options(NodeGraphCore
        PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wno-unused-parameter
    )
endif()

# ─────────────────────────────────────
# 8. Main application target
# ─────────────────────────────────────
add_executable(NodeGraph WIN32 ${MAIN_SOURCES})

# Include directories (needed for libxml2 headers)
target_include_directories(NodeGraph
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link to core library and Qt
target_link_libraries(NodeGraph
    PRIVATE
        NodeGraphCore
        Qt5::Core
        Qt5::Widgets
        Qt5::Gui
        Qt5::Qml
        LibXml2::LibXml2
)

# Windows-specific settings
if(WIN32)
    set_target_properties(NodeGraph PROPERTIES
        WIN32_EXECUTABLE TRUE
        VS_DPI_AWARE "PerMonitor"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
        VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    )

    # Set environment for Debug configuration
    set_target_properties(NodeGraph PROPERTIES
        VS_DEBUGGER_ENVIRONMENT "PATH=E:/Qt/5.15.16/$<IF:$<CONFIG:Debug>,Debug_x64,Release_x64>/bin;%PATH%"
    )

    # Windows-specific linker optimizations
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        set_target_properties(NodeGraph PROPERTIES
            LINK_FLAGS "/LTCG /INCREMENTAL:NO /OPT:REF /OPT:ICF"
        )
    endif()

    # Additional Windows-specific optimizations
    set_target_properties(NodeGraph PROPERTIES
        LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS,5.02 /ENTRY:mainCRTStartup"
    )
endif()

# ─────────────────────────────────────
# 9. Install rules
# ─────────────────────────────────────
if(WIN32)
    install(TARGETS NodeGraph
        RUNTIME DESTINATION bin
    )

    # Install Qt DLLs (simplified approach)
    install(CODE "
        file(INSTALL
            \"${QT5_PATH}/bin/Qt5Core.dll\"
            \"${QT5_PATH}/bin/Qt5Widgets.dll\"
            \"${QT5_PATH}/bin/Qt5Gui.dll\"
            DESTINATION \"${CMAKE_INSTALL_PREFIX}/bin\"
        )
    ")
endif()

# ─────────────────────────────────────
# 10. CI Guardrails - Architectural lint rules
# ─────────────────────────────────────
add_custom_target(lint_guard
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/lint_guard.sh
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running architectural lint rules"
    VERBATIM
)

# ─────────────────────────────────────
# 11. Build information
# ─────────────────────────────────────
message(STATUS "")
message(STATUS "─────────────────────────────────────")
message(STATUS "Build Configuration:")
message(STATUS "  Project:          ${PROJECT_NAME} ${PROJECT_VERSION}")
message(STATUS "  Build Type:       ${CMAKE_BUILD_TYPE}")
message(STATUS "  Qt Path:          ${QT5_PATH}")
message(STATUS "  C++ Standard:     C++${CMAKE_CXX_STANDARD}")
message(STATUS "  Qt Version:       ${Qt5_VERSION}")
message(STATUS "  libxml2 Source:   FetchContent")
message(STATUS "  Generator:        ${CMAKE_GENERATOR}")
message(STATUS "  Install Prefix:   ${CMAKE_INSTALL_PREFIX}")
message(STATUS "─────────────────────────────────────")
message(STATUS "")
