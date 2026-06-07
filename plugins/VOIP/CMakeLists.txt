# plugins/VOIP/CMakeLists.txt
cmake_minimum_required(VERSION 3.18)
project(VOIPPlugin)

# --- Dependencies ---
find_package(Qt5 COMPONENTS Core Widgets Multimedia Xml Network REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(AVCODEC REQUIRED libavcodec)
pkg_check_modules(AVUTIL REQUIRED libavutil)
pkg_check_modules(SPEEX REQUIRED speex)
pkg_check_modules(SPEEXDSP REQUIRED speexdsp)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# --- Definitions ---

# --- Sources ---
set(VOIP_SOURCES
    VOIPPlugin.cpp
    gui/VOIPConfigPanel.cpp
    services/p3VOIP.cc
    services/rsVOIPItems.cc
    gui/AudioStats.cpp
    gui/AudioWizard.cpp
    gui/SpeexProcessor.cpp
    gui/audiodevicehelper.cpp
    gui/VideoProcessor.cpp
    gui/QVideoDevice.cpp
    gui/VOIPChatWidgetHolder.cpp
    gui/VOIPGUIHandler.cpp
    gui/VOIPNotify.cpp
    gui/VOIPToasterItem.cpp
    gui/VOIPToasterNotify.cpp
)

# RsTurtleVOIPBridge implements distant (gxsid<->gxsid) VOIP. It is now a client
# of p3GxsTunnelService (libretroshare) for the encrypted E2E transport and no
# longer uses OpenSSL directly, so no extra crypto link is required. It is still
# added conditionally so the plugin builds on a tree without the bridge (e.g.
# master).
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/services/RsTurtleVOIPBridge.cpp")
    list(APPEND VOIP_SOURCES services/RsTurtleVOIPBridge.cpp)
endif()

set(VOIP_FORMS
    gui/AudioStats.ui
    gui/AudioWizard.ui
    gui/VOIPConfigPanel.ui
    gui/VOIPToasterItem.ui
)

set(VOIP_RESOURCES
    gui/VOIP_images.qrc
    lang/VOIP_lang.qrc
    qss/VOIP_qss.qrc
)

# --- Translations (.ts -> .qm) ---
file(GLOB TS_FILES "lang/*.ts")
if(Qt5LinguistTools_FOUND)
    qt5_add_translation(QM_FILES ${TS_FILES})
else()
    find_program(LRELEASE_EXECUTABLE NAMES lrelease-qt5 lrelease)
    foreach(ts_file ${TS_FILES})
        get_filename_component(ts_name ${ts_file} NAME_WE)
        set(qm_src_file "${CMAKE_CURRENT_SOURCE_DIR}/lang/${ts_name}.qm")
        set(qm_bin_file "${CMAKE_CURRENT_BINARY_DIR}/${ts_name}.qm")
        
        if(EXISTS ${qm_src_file})
            list(APPEND QM_FILES ${qm_src_file})
        elseif(LRELEASE_EXECUTABLE)
            add_custom_command(
                OUTPUT ${qm_bin_file}
                COMMAND ${LRELEASE_EXECUTABLE} ${ts_file} -qm ${qm_bin_file}
                DEPENDS ${ts_file}
                VERBATIM
            )
            list(APPEND QM_FILES ${qm_bin_file})
        endif()
    endforeach()
endif()

# --- Target ---
add_library(VOIP MODULE
    ${VOIP_SOURCES}
    ${VOIP_FORMS}
    ${VOIP_RESOURCES}
    ${QM_FILES}
)

set_target_properties(VOIP PROPERTIES
    PREFIX "lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins"
    AUTOUIC_SEARCH_PATHS "${CMAKE_CURRENT_SOURCE_DIR}/gui"
)

# FFmpeg/libav headers (libavcodec/libavutil, included by gui/VideoProcessor.*)
# use the C99 fixed-width constant macros (INT64_C, UINT64_C, ...). In a C++
# translation unit these are only exposed by <stdint.h> when
# __STDC_CONSTANT_MACROS is defined before it is included, so it must be defined
# for the whole target to compile the libav headers.
target_compile_definitions(VOIP PRIVATE __STDC_CONSTANT_MACROS)
if(WIN32)
    target_compile_definitions(VOIP PRIVATE WINDOWS_SYS)
endif()

# --- Links ---
if(WIN32)
    # See FeedReader/CMakeLists.txt: on Windows a plugin DLL must resolve every
    # symbol at link time; on Linux they are resolved from the host process.
    target_link_libraries(VOIP PRIVATE retroshare retroshare-gui)
endif()

# pkg-config gives the FFmpeg/speex libs by name only (avcodec, avutil, ...);
# their directory must be added explicitly or the linker won't find them when
# they live outside the default search path (e.g. Homebrew /opt/homebrew/... on
# macOS -> "ld: library 'avcodec' not found"). Harmless on Linux/Windows where
# these dirs are already on the default path.
target_link_directories(VOIP PRIVATE
    ${AVCODEC_LIBRARY_DIRS}
    ${AVUTIL_LIBRARY_DIRS}
    ${SPEEX_LIBRARY_DIRS}
    ${SPEEXDSP_LIBRARY_DIRS}
)

# NOTE: the VOIP plugin makes no direct use of OpenSSL. All cryptography for
# distant VOIP is delegated to libretroshare's p3GxsTunnelService, so the plugin
# stays OpenSSL-free.
target_link_libraries(VOIP PRIVATE
    Qt5::Core
    Qt5::Widgets
    Qt5::Multimedia
    Qt5::Xml
    Qt5::Network
    ${AVCODEC_LIBRARIES}
    ${AVUTIL_LIBRARIES}
    ${SPEEX_LIBRARIES}
    ${SPEEXDSP_LIBRARIES}
)

target_compile_options(VOIP PRIVATE "-Wno-deprecated-declarations")
target_compile_definitions(VOIP PRIVATE RS_NO_WARN_DEPRECATED)

if(RS_DEVELOPMENT_BUILD)
    target_compile_options(VOIP PRIVATE "-g" "-O0")
endif()

# --- Includes ---
target_include_directories(VOIP PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/gui
    ${CMAKE_CURRENT_SOURCE_DIR}/services
    ${CMAKE_CURRENT_SOURCE_DIR}/interface
    ${CMAKE_SOURCE_DIR}/libretroshare/src
    ${CMAKE_SOURCE_DIR}/retroshare-gui/src
    ${CMAKE_SOURCE_DIR}/retroshare-gui/src/gui
    ${CMAKE_SOURCE_DIR}/supportlibs/rapidjson/include
    ${AVCODEC_INCLUDE_DIRS}
    ${AVUTIL_INCLUDE_DIRS}
    ${SPEEX_INCLUDE_DIRS}
    ${SPEEXDSP_INCLUDE_DIRS}
)

# --- Installation ---
install(TARGETS VOIP DESTINATION lib/retroshare/plugins)
