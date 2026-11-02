# plugins/FeedReader/CMakeLists.txt
cmake_minimum_required(VERSION 3.18)
project(FeedReaderPlugin)

# Version-agnostic Qt detection: probe Qt6 first, fall back to Qt5, mirroring
# retroshare-gui/CMakeLists.txt so the plugin builds against whichever Qt the GUI
# uses. Do NOT use find_package(QT NAMES Qt6 Qt5 ...): on multiarch distros that
# ship both, it picks Qt5 in filesystem order. The explicit probe is deterministic.
find_package(Qt6 QUIET COMPONENTS Core)
if(Qt6_FOUND)
    set(QT_VERSION_MAJOR 6)
else()
    find_package(Qt5 COMPONENTS Core REQUIRED)
    set(QT_VERSION_MAJOR 5)
endif()
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core Widgets Xml Network REQUIRED)
if(QT_VERSION_MAJOR EQUAL 6)
    # Core5Compat provides QRegExp/QTextCodec etc. still referenced by the shared
    # retroshare-gui headers this plugin includes; matches the GUI's own link.
    find_package(Qt6 COMPONENTS Core5Compat REQUIRED)
endif()

# --- Dependencies ---
find_package(CURL REQUIRED)
find_package(LibXml2 REQUIRED)
find_package(LibXslt REQUIRED)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# --- Sources ---
set(FEEDREADER_SOURCES
    FeedReaderPlugin.cpp
    services/p3FeedReader.cc
    services/p3FeedReaderThread.cc
    services/rsFeedReaderItems.cc
    gui/FeedReaderDialog.cpp
    gui/FeedReaderMessageWidget.cpp
    gui/AddFeedDialog.cpp
    gui/PreviewFeedDialog.cpp
    gui/FeedReaderNotify.cpp
    gui/FeedReaderConfig.cpp
    gui/FeedReaderStringDefs.cpp
    gui/FeedReaderFeedNotify.cpp
    gui/FeedReaderUserNotify.cpp
    gui/FeedReaderFeedItem.cpp
    gui/FeedTreeWidget.cpp
    gui/ProxyWidget.cpp
    util/CURLWrapper.cpp
    util/XMLWrapper.cpp
    util/HTMLWrapper.cpp
    util/XPathWrapper.cpp
)

set(FEEDREADER_FORMS
    gui/FeedReaderDialog.ui
    gui/FeedReaderMessageWidget.ui
    gui/AddFeedDialog.ui
    gui/PreviewFeedDialog.ui
    gui/FeedReaderConfig.ui
    gui/FeedReaderFeedItem.ui
    gui/ProxyWidget.ui
)

set(FEEDREADER_RESOURCES
    gui/FeedReader_images.qrc
    lang/FeedReader_lang.qrc
    qss/FeedReader_qss.qrc
)

# --- Translations (.ts -> .qm) ---
file(GLOB TS_FILES "lang/*.ts")
if(Qt${QT_VERSION_MAJOR}LinguistTools_FOUND)
    qt_add_translation(QM_FILES ${TS_FILES})
else()
    find_program(LRELEASE_EXECUTABLE NAMES lrelease-qt${QT_VERSION_MAJOR} lrelease-qt5 lrelease)
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

# --- CTarget ---
add_library(FeedReader MODULE
    ${FEEDREADER_SOURCES}
    ${FEEDREADER_FORMS}
    ${FEEDREADER_RESOURCES}
    ${QM_FILES}
)

set_target_properties(FeedReader PROPERTIES
    PREFIX "lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins"
    AUTOUIC_SEARCH_PATHS "${CMAKE_CURRENT_SOURCE_DIR}/gui"
)

# --- Links ---
# OpenSSL is a direct dependency: services/p3FeedReaderThread.cc includes
# <openssl/evp.h> and uses the EVP API directly (not only through libretroshare).
find_package(OpenSSL REQUIRED)

if(WIN32)
    # On Windows a shared module (plugin DLL) must resolve every symbol at link
    # time (no undefined symbols allowed in a PE/DLL), so the plugin has to link
    # explicitly against the libraries that provide them. On ELF platforms
    # (Linux) the loader resolves these symbols from the hosting retroshare-gui
    # process at load time, so this explicit link is only needed on Windows.
    target_link_libraries(FeedReader PRIVATE retroshare retroshare-gui)
    target_compile_definitions(FeedReader PRIVATE WINDOWS_SYS)
endif()

target_link_libraries(FeedReader PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Xml
    Qt${QT_VERSION_MAJOR}::Network
    CURL::libcurl
    LibXml2::LibXml2
    LibXslt::LibXslt
    OpenSSL::SSL
    OpenSSL::Crypto
)
if(QT_VERSION_MAJOR EQUAL 6)
    target_link_libraries(FeedReader PRIVATE Qt6::Core5Compat)
endif()

target_compile_options(FeedReader PRIVATE "-Wno-deprecated-declarations")
target_compile_definitions(FeedReader PRIVATE RS_NO_WARN_DEPRECATED)

if(RS_DEVELOPMENT_BUILD)
    target_compile_options(FeedReader PRIVATE "-g" "-O0")
endif()

# --- Includes ---
target_include_directories(FeedReader PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/gui
    ${CMAKE_CURRENT_SOURCE_DIR}/services
    ${CMAKE_CURRENT_SOURCE_DIR}/interface
    ${CMAKE_CURRENT_SOURCE_DIR}/util
    ${CMAKE_SOURCE_DIR}/libretroshare/src
    ${CMAKE_SOURCE_DIR}/retroshare-gui/src
    ${CMAKE_SOURCE_DIR}/retroshare-gui/src/gui
    ${CMAKE_SOURCE_DIR}/supportlibs/rapidjson/include
)

# --- Installation ---
install(TARGETS FeedReader DESTINATION lib/retroshare/plugins)
