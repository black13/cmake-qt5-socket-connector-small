cmake_minimum_required(VERSION 3.16)
project(NodeGraph VERSION 1.0.0 LANGUAGES CXX)
include(CTest)

# ─────────────────────────────────────
# NOTE: Code Review Compendium Script
# ─────────────────────────────────────
# generate_code_compendium.sh - Creates complete code review package
# This script is NOT part of the build - it's for reviewer delivery only
# Run: ./generate_code_compendium.sh to create reviewer package

# ─────────────────────────────────────
# 1.  C++ standard
# ─────────────────────────────────────
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ─────────────────────────────────────
# 1a. AddressSanitizer for memory debugging (Linux/WSL only)
# ─────────────────────────────────────
if(CMAKE_BUILD_TYPE STREQUAL "Debug" AND NOT WIN32)
    message(STATUS "Enabling AddressSanitizer for memory debugging")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer -g")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")
endif()

# ─────────────────────────────────────
# 2.  Windows-specific defines
# ─────────────────────────────────────
if(WIN32)
    set(CMAKE_SYSTEM_VERSION 10.0)
    add_definitions(-DWIN32_LEAN_AND_MEAN -DNOMINMAX)
endif()

# ─────────────────────────────────────
# 3.  Default build-type
# ─────────────────────────────────────
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# ─────────────────────────────────────
# 4.  Qt search paths (adapt to your install)
# ─────────────────────────────────────
if(WIN32)
    # Order matters! CMake uses first match.
    # For Debug builds, list Debug path first; for Release, list Release first
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        list(APPEND CMAKE_PREFIX_PATH
            "D:/Qt/5.15.19/debug"                              # Debug first for Debug builds
            "D:/Qt/5.15.19/release"
            "C:/Qt/5.15.2/msvc2019_64"
            "C:/Qt/5.15.2/msvc2019"
        )
        message(STATUS "Debug build: Prioritizing Qt Debug libraries at D:/Qt/5.15.19/debug")
    else()
        list(APPEND CMAKE_PREFIX_PATH
            "D:/Qt/5.15.19/release"                            # Release first for Release builds
            "D:/Qt/5.15.19/debug"
            "C:/Qt/5.15.2/msvc2019_64"
            "C:/Qt/5.15.2/msvc2019"
        )
        message(STATUS "Release build: Prioritizing Qt Release libraries at D:/Qt/5.15.19/release")
    endif()
else()
    # Linux/WSL: Auto-detect Qt installations in /usr/local/qt-*
    file(GLOB QT_INSTALL_DIRS "/usr/local/qt*")
    
    # Sort directories to prefer newer versions (reverse alphabetical)
    list(SORT QT_INSTALL_DIRS)
    list(REVERSE QT_INSTALL_DIRS)
    
    # Add found Qt installations to search path
    foreach(QT_DIR ${QT_INSTALL_DIRS})
        if(IS_DIRECTORY "${QT_DIR}")
            # Check for common Qt directory structures
            if(EXISTS "${QT_DIR}/lib/cmake/Qt5")
                list(APPEND CMAKE_PREFIX_PATH "${QT_DIR}")
                message(STATUS "Found Qt installation: ${QT_DIR}")
            endif()
            
            # Check for debug/release subdirectories
            if(EXISTS "${QT_DIR}/debug" AND EXISTS "${QT_DIR}/debug/lib/cmake/Qt5")
                list(APPEND CMAKE_PREFIX_PATH "${QT_DIR}/debug")
                message(STATUS "Found Qt Debug build: ${QT_DIR}/debug")
            endif()
            
            if(EXISTS "${QT_DIR}/release" AND EXISTS "${QT_DIR}/release/lib/cmake/Qt5")
                list(APPEND CMAKE_PREFIX_PATH "${QT_DIR}/release")
                message(STATUS "Found Qt Release build: ${QT_DIR}/release")
            endif()
        endif()
    endforeach()
    
    # Prefer debug builds for Debug configuration, release builds for Release
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        message(STATUS "Debug build: Preferring Qt debug libraries")
    else()
        message(STATUS "Release build: Preferring Qt release libraries")
    endif()
endif()

# ─────────────────────────────────────
# 5.  Find Qt5 modules
# ─────────────────────────────────────
# Qml is required for QJSEngine (JavaScript integration in Graph facade);
# Svg backs the qsvg icon-image plugin for the app's SVG action icons.
set(QT5_COMPONENTS Core Widgets Gui Qml Network Svg)
if(BUILD_TESTING)
    list(APPEND QT5_COMPONENTS Test)
endif()

find_package(Qt5 REQUIRED COMPONENTS ${QT5_COMPONENTS})

# ─────────────────────────────────────
# Find or build libxml2 for cross-platform XML handling
# ─────────────────────────────────────

# Try to use system libxml2 first (Linux/Unix only), fallback to FetchContent
set(USE_SYSTEM_LIBXML2 FALSE)

if(NOT WIN32)
    find_package(PkgConfig QUIET)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(LIBXML2 QUIET libxml-2.0>=2.9.0)
        if(LIBXML2_FOUND AND LIBXML2_INCLUDE_DIRS)
            # Verify that we can actually find the headers
            find_path(LIBXML2_HEADER_TEST libxml/tree.h PATHS ${LIBXML2_INCLUDE_DIRS} NO_DEFAULT_PATH)
            if(LIBXML2_HEADER_TEST)
                set(USE_SYSTEM_LIBXML2 TRUE)
            endif()
        endif()
    endif()

    # Fallback 1: CMake's FindLibXml2 module (uses xml2-config when available)
    if(NOT USE_SYSTEM_LIBXML2)
        find_package(LibXml2 QUIET)
        if(LIBXML2_FOUND)
            set(USE_SYSTEM_LIBXML2 TRUE)
        endif()
    endif()

    # Fallback 2: Manual xml2-config detection (defines imported target)
    if(NOT USE_SYSTEM_LIBXML2)
        find_program(XML2_CONFIG NAMES xml2-config)
        if(XML2_CONFIG)
            execute_process(COMMAND ${XML2_CONFIG} --cflags OUTPUT_VARIABLE XML2_CFLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)
            execute_process(COMMAND ${XML2_CONFIG} --libs   OUTPUT_VARIABLE XML2_LIBS   OUTPUT_STRIP_TRAILING_WHITESPACE)
            separate_arguments(XML2_CFLAGS)
            separate_arguments(XML2_LIBS)
            add_library(LibXml2::LibXml2 INTERFACE IMPORTED)
            target_compile_options(LibXml2::LibXml2 INTERFACE ${XML2_CFLAGS})
            target_link_libraries(LibXml2::LibXml2 INTERFACE ${XML2_LIBS})
            set(USE_SYSTEM_LIBXML2 TRUE)
            message(STATUS "Using system libxml2 via xml2-config")
        endif()
    endif()
endif()

if(USE_SYSTEM_LIBXML2)
    message(STATUS "Using system libxml2")
    # Create imported target only if not already defined (xml2-config fallback defines it)
    if(NOT TARGET LibXml2::LibXml2)
        add_library(LibXml2::LibXml2 INTERFACE IMPORTED)
        if(DEFINED LIBXML2_LIBRARIES)
            target_link_libraries(LibXml2::LibXml2 INTERFACE ${LIBXML2_LIBRARIES})
        endif()
        if(DEFINED LIBXML2_INCLUDE_DIRS)
            target_include_directories(LibXml2::LibXml2 INTERFACE ${LIBXML2_INCLUDE_DIRS})
        endif()
        if(DEFINED LIBXML2_CFLAGS_OTHER)
            target_compile_options(LibXml2::LibXml2 INTERFACE ${LIBXML2_CFLAGS_OTHER})
        endif()
    endif()
else()
    # Use FetchContent with persistent global cache
    include(FetchContent)
    
    # Set a global cache directory that persists across builds (separate for Debug/Release)
    set(FETCHCONTENT_BASE_DIR "${CMAKE_SOURCE_DIR}/.cmake-cache/${CMAKE_BUILD_TYPE}" CACHE PATH "FetchContent cache directory")
    
    # Check if libxml2 is already populated to avoid rebuilding
    FetchContent_GetProperties(libxml2)
    if(NOT libxml2_POPULATED)
        if(WIN32)
            message(STATUS "Windows: Building libxml2 from source (cached) - downloading and building...")
        else()
            message(STATUS "libxml2 not found in persistent cache - downloading and building...")
        endif()
        
        FetchContent_Declare(
            libxml2
            GIT_REPOSITORY https://github.com/GNOME/libxml2.git
            GIT_TAG        v2.12.5  # Stable version
            GIT_SHALLOW    TRUE
            SOURCE_DIR     "${FETCHCONTENT_BASE_DIR}/libxml2-src"
            BINARY_DIR     "${FETCHCONTENT_BASE_DIR}/libxml2-build"
        )
        
        # Configure libxml2 build options - minimal features for performance
        set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries" FORCE)
        set(LIBXML2_WITH_ICONV OFF CACHE BOOL "Build with iconv support" FORCE)
        set(LIBXML2_WITH_LZMA OFF CACHE BOOL "Build with lzma support" FORCE) 
        set(LIBXML2_WITH_ZLIB OFF CACHE BOOL "Build with zlib support" FORCE)
        set(LIBXML2_WITH_PYTHON OFF CACHE BOOL "Build Python bindings" FORCE)
        set(LIBXML2_WITH_TESTS OFF CACHE BOOL "Build tests" FORCE)
        set(LIBXML2_WITH_PROGRAMS OFF CACHE BOOL "Build programs" FORCE)
        set(LIBXML2_WITH_HTTP OFF CACHE BOOL "Build with HTTP support" FORCE)
        set(LIBXML2_WITH_FTP OFF CACHE BOOL "Build with FTP support" FORCE)
        
        FetchContent_MakeAvailable(libxml2)
        message(STATUS "libxml2 build complete - cached in ${FETCHCONTENT_BASE_DIR}")
    else()
        message(STATUS "libxml2 found in persistent cache - skipping rebuild")
    endif()
endif()

# ─────────────────────────────────────
# Note: Graaf integration disabled for now due to heavy dependencies
# Will use lightweight custom layout implementation instead
# ─────────────────────────────────────

# Enable libxml2 for all platforms
add_definitions(-DUSE_LIBXML2)

# auto-moc for QObject / signals
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# ─────────────────────────────────────
# 6.  MSVC compiler options
# ─────────────────────────────────────
if(MSVC)
    add_compile_options(/W3 /MP)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS -DUNICODE -D_UNICODE)
endif()

# ─────────────────────────────────────
# 7.  Shared core library to avoid double building
# ─────────────────────────────────────
set(CORE_SOURCES
    # Self-serializing architecture
    node.h
    node.cpp
    scripted_node.h
    scripted_node.cpp
    socket.h
    socket.cpp
    edge.h
    edge.cpp
    ghost_edge.h
    ghost_edge.cpp
    synthetic_work.h
    synthetic_work.cpp

    # Opt-in verbose logging category (default off; see nodegraph_logging.h)
    nodegraph_logging.h
    nodegraph_logging.cpp
    
    
    # XML-first factory
    graph_factory.h
    graph_factory.cpp

    # Type-erased scripting seam (Sean Parent runtime-concept idiom)
    script_engine.h
    qjs_script_backend.h
    qjs_script_backend.cpp
    duktape_script_backend.h
    duktape_script_backend.cpp   # no-op unless ENABLE_DUKTAPE=ON

    # Undo/redo commands (XML-snapshot based)
    undo_commands.h
    undo_commands.cpp

    # Graph facade (public API with JavaScript integration)
    graph.h
    graph.cpp

    # Template-driven scriptable node types
    node_templates.h
    node_templates.cpp
    
    # Layout engine using Graaf (disabled for now)
    # layout_engine.h
    # layout_engine.cpp
    
    # UI classes
    window.h
    window.cpp
    view.h
    view.cpp
    scene.h
    scene.cpp
    
    # Professional drag-and-drop palette
    node_palette_widget.h
    node_palette_widget.cpp
    
    # Observer pattern implementation
    graph_observer.h
    graph_observer.cpp
    xml_autosave_observer.h
    xml_autosave_observer.cpp

    # Live XML synchronization (not implemented yet)
    # xml_live_sync.h
    # xml_live_sync.cpp
)

# Main application sources (only main.cpp + resources)
set(MAIN_SOURCES
    main.cpp
    icons.qrc
)


# ─────────────────────────────────────
# 8.  Shared core library - compile once, use twice
# ─────────────────────────────────────
add_library(NodeGraphCore STATIC ${CORE_SOURCES})
target_include_directories(NodeGraphCore PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

# ─────────────────────────────────────
# 9.  Main executable target
# ─────────────────────────────────────
add_executable(NodeGraph WIN32 ${MAIN_SOURCES})
target_link_libraries(NodeGraph PRIVATE NodeGraphCore)

# ─────────────────────────────────────
# 10. Headless regression tests
# ─────────────────────────────────────
if(BUILD_TESTING)
    add_executable(NodeGraphTests tests/tst_main.cpp)
    target_link_libraries(NodeGraphTests PRIVATE NodeGraphCore Qt5::Test)
    add_executable(NodeGraphLoadingTests tests/tst_loading.cpp)
    target_link_libraries(NodeGraphLoadingTests PRIVATE NodeGraphCore Qt5::Test)
    foreach(test_target NodeGraphTests NodeGraphLoadingTests)
        if(WIN32)
            add_test(NAME ${test_target} COMMAND ${CMAKE_COMMAND} -E env
                "QT_QPA_PLATFORM=offscreen"
                "QT_QPA_PLATFORM_PLUGIN_PATH=$<TARGET_FILE_DIR:Qt5::Core>/../plugins/platforms"
                "PATH=$<TARGET_FILE_DIR:Qt5::Core>;$ENV{PATH}"
                $<TARGET_FILE:${test_target}>)
        else()
            add_test(NAME ${test_target} COMMAND ${test_target})
            set_tests_properties(${test_target} PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
        endif()
        set_tests_properties(${test_target} PROPERTIES TIMEOUT 60)
    endforeach()
endif()

# ─────────────────────────────────────
# Optional Duktape script engine backend
# Select at runtime with NODEGRAPH_SCRIPT_ENGINE=duktape
# ─────────────────────────────────────
option(ENABLE_DUKTAPE "Build the Duktape backend for the ScriptEngine seam" OFF)
if(ENABLE_DUKTAPE)
    include(FetchContent)
    enable_language(C)
    FetchContent_Declare(
        duktape
        URL https://github.com/svaarala/duktape/releases/download/v2.7.0/duktape-2.7.0.tar.xz
    )
    FetchContent_MakeAvailable(duktape)
    add_library(duktape_lib STATIC ${duktape_SOURCE_DIR}/src/duktape.c)
    target_include_directories(duktape_lib PUBLIC ${duktape_SOURCE_DIR}/src)
    target_compile_definitions(NodeGraphCore PUBLIC NODEGRAPH_HAS_DUKTAPE)
    message(STATUS "Duktape backend  : ENABLED (v2.7.0 via FetchContent)")
else()
    message(STATUS "Duktape backend  : disabled (configure with -DENABLE_DUKTAPE=ON)")
endif()

# Core library linking
set(CORE_LIBRARIES
    Qt5::Core
    Qt5::Widgets
    Qt5::Gui
    Qt5::Qml  # Required for QJSEngine in Graph facade
    LibXml2::LibXml2
)

if(ENABLE_DUKTAPE)
    list(APPEND CORE_LIBRARIES duktape_lib)
endif()

target_link_libraries(NodeGraphCore ${CORE_LIBRARIES})

# ─────────────────────────────────────
# 11a. LLVM source-based code coverage instrumentation
# ─────────────────────────────────────
option(ENABLE_LLVM_COVERAGE "Enable LLVM source-based code coverage instrumentation" OFF)
if(ENABLE_LLVM_COVERAGE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        set(COVERAGE_FLAGS -fprofile-instr-generate -fcoverage-mapping)
        target_compile_options(NodeGraphCore PUBLIC ${COVERAGE_FLAGS})
        target_compile_options(NodeGraph PRIVATE ${COVERAGE_FLAGS})

        # Find LLVM profiling runtime library (needed for lld-link.exe which
        # does not understand -fprofile-instr-generate as a linker flag)
        if(WIN32)
            # clang-cl + Ninja uses lld-link.exe directly, not clang-cl as linker driver
            # We must explicitly link clang_rt.profile-x86_64.lib
            execute_process(
                COMMAND "${CMAKE_CXX_COMPILER}" -print-resource-dir
                OUTPUT_VARIABLE CLANG_RESOURCE_DIR
                OUTPUT_STRIP_TRAILING_WHITESPACE
                ERROR_QUIET
            )
            if(CLANG_RESOURCE_DIR)
                set(PROFILE_RT_LIB "${CLANG_RESOURCE_DIR}/lib/windows/clang_rt.profile-x86_64.lib")
                if(EXISTS "${PROFILE_RT_LIB}")
                    message(STATUS "LLVM profiling runtime: ${PROFILE_RT_LIB}")
                else()
                    # Fallback: search under LLVM install
                    find_library(PROFILE_RT_LIB
                        NAMES clang_rt.profile-x86_64
                        PATHS "${CLANG_RESOURCE_DIR}/lib/windows"
                        NO_DEFAULT_PATH
                    )
                endif()
            endif()
            if(NOT PROFILE_RT_LIB OR NOT EXISTS "${PROFILE_RT_LIB}")
                message(WARNING "LLVM profiling runtime not found - coverage .profraw files will NOT be written at runtime")
            else()
                target_link_libraries(NodeGraphCore PUBLIC "${PROFILE_RT_LIB}")
            endif()
        else()
            target_link_options(NodeGraphCore INTERFACE -fprofile-instr-generate)
        endif()

        message(STATUS "LLVM source-based code coverage: ENABLED")
        message(STATUS "  Runtime:  set LLVM_PROFILE_FILE=nodegraph_%%p_%%m.profraw && NodeGraph.exe")
        message(STATUS "  Merge:    llvm-profdata merge -sparse *.profraw -o merged.profdata")
        message(STATUS "  Report:   llvm-cov show NodeGraph.exe -instr-profile=merged.profdata")
        message(STATUS "  HTML:     llvm-cov show NodeGraph.exe -instr-profile=merged.profdata -format=html -output-dir=coverage_report")
    else()
        message(WARNING "ENABLE_LLVM_COVERAGE requires Clang compiler (detected: ${CMAKE_CXX_COMPILER_ID})")
    endif()
endif()

# ─────────────────────────────────────
# 12. VS / Windows extras
# ─────────────────────────────────────
if(WIN32)
    # Main application settings
    set_target_properties(NodeGraph PROPERTIES
        WIN32_EXECUTABLE TRUE
        VS_DPI_AWARE "PerMonitor"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"

        # Configure Qt PATH for Visual Studio debugger (configuration-specific)
        VS_DEBUGGER_ENVIRONMENT "PATH=$<IF:$<CONFIG:Debug>,D:/Qt/5.15.19/debug/bin,D:/Qt/5.15.19/release/bin>;%PATH%"

        # No command arguments - removed test_working_graph.xml
        VS_DEBUGGER_WORKING_DIRECTORY
            "${CMAKE_CURRENT_SOURCE_DIR}"
    )
    
    # Test application settings DISABLED
    # set_target_properties(NodeGraphTests PROPERTIES
    #     WIN32_EXECUTABLE FALSE
    #     RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    #     
    #     # Separate Debug and Release Qt paths for tests
    #     VS_DEBUGGER_ENVIRONMENT_DEBUG
    #         "PATH=E:/Qt/5.15.16/Debug_x64/bin;%PATH%"
    #     VS_DEBUGGER_ENVIRONMENT_RELEASE
    #         "PATH=E:/Qt/5.15.16/Release_x64/bin;%PATH%"
    #         
    #     VS_DEBUGGER_WORKING_DIRECTORY
    #         "${CMAKE_CURRENT_SOURCE_DIR}"
    # )
    
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                 PROPERTY VS_STARTUP_PROJECT NodeGraph)

    # Make the build tree directly runnable: copy the Qt runtime (libraries +
    # plugins) next to NodeGraph.exe so no PATH or plugin setup is needed for:
    #   build_Release\Release\NodeGraph.exe --script scripts/drop_demo.js
    # The debug-only Qt install trips windeployqt's suffix logic (it looks for
    # Qt5Coredd.dll), so windeployqt deploys plugins only and the libraries are
    # copied with copy_if_different from the imported targets.
    add_custom_command(TARGET NodeGraph POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:Qt5::Core>
                $<TARGET_FILE:Qt5::Gui>
                $<TARGET_FILE:Qt5::Widgets>
                $<TARGET_FILE:Qt5::Qml>
                $<TARGET_FILE:Qt5::Network>
                $<TARGET_FILE:Qt5::Svg>
                "$<TARGET_FILE_DIR:NodeGraph>"
        COMMAND "$<TARGET_FILE_DIR:Qt5::Core>/windeployqt.exe"
                $<IF:$<CONFIG:Debug>,--debug,--release>
                --no-libraries
                --no-translations
                --no-compiler-runtime
                --no-system-d3d-compiler
                --no-opengl-sw
                --no-angle
                --no-virtualkeyboard
                --no-quick
                --no-quick-import
                "$<TARGET_FILE:NodeGraph>"
        COMMENT "Deploying Qt runtime next to NodeGraph.exe"
        VERBATIM)
endif()

# ─────────────────────────────────────
# 13. Install rules (optional on Win)
# ─────────────────────────────────────
if(WIN32)
    install(TARGETS NodeGraph RUNTIME DESTINATION .)

    # Qt runtime DLLs the app actually imports. Qt5Qml (QJSEngine) pulls in
    # Qt5Network; Qt5Svg backs the qsvg icon-image plugin.
    install(FILES
        $<TARGET_FILE:Qt5::Core>
        $<TARGET_FILE:Qt5::Widgets>
        $<TARGET_FILE:Qt5::Gui>
        $<TARGET_FILE:Qt5::Qml>
        $<TARGET_FILE:Qt5::Network>
        $<TARGET_FILE:Qt5::Svg>
        DESTINATION .
    )

    # A GUI app cannot start without the platform plugin. Derive the plugin
    # directory from the imported Qt5::Core location; debug plugins use a
    # 'd' suffix (qwindowsd.dll).
    get_target_property(_qt_core_dll Qt5::Core IMPORTED_LOCATION)
    if(NOT _qt_core_dll)
        get_target_property(_qt_core_dll Qt5::Core IMPORTED_LOCATION_DEBUG)
    endif()
    if(NOT _qt_core_dll)
        get_target_property(_qt_core_dll Qt5::Core IMPORTED_LOCATION_RELEASE)
    endif()
    if(_qt_core_dll)
        get_filename_component(_qt_bin_dir "${_qt_core_dll}" DIRECTORY)
        get_filename_component(_qt_plugins_dir "${_qt_bin_dir}/../plugins" ABSOLUTE)
        install(FILES "${_qt_plugins_dir}/platforms/qwindows$<IF:$<CONFIG:Debug>,d,>.dll"
                DESTINATION platforms)
        install(FILES "${_qt_plugins_dir}/imageformats/qsvg$<IF:$<CONFIG:Debug>,d,>.dll"
                DESTINATION imageformats)
    else()
        message(WARNING "Could not locate Qt5::Core DLL; install the platform plugin with windeployqt")
    endif()
endif()

# ─────────────────────────────────────
# 14. Info
# ─────────────────────────────────────
message(STATUS "Build type       : ${CMAKE_BUILD_TYPE}")
message(STATUS "Qt5 found        : ${Qt5_FOUND}")
message(STATUS "Qt5 version      : ${Qt5_VERSION}")
message(STATUS "Qt5 location     : ${Qt5_DIR}")

# Cache information
if(USE_SYSTEM_LIBXML2)
    message(STATUS "libxml2 source   : System package")
else()
    FetchContent_GetProperties(libxml2)
    if(libxml2_POPULATED)
        message(STATUS "libxml2 source   : FetchContent (cached)")
    else()
        if(WIN32)
            message(STATUS "libxml2 source   : FetchContent (will build for Windows)")
        else()
            message(STATUS "libxml2 source   : FetchContent (will download)")
        endif()
    endif()
endif()
