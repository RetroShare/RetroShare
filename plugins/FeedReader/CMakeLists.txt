# plugins/FeedReader/CMakeLists.txt
cmake_minimum_required(VERSION 3.18)
project(FeedReaderPlugin)

find_package(Qt5 COMPONENTS Core Widgets Xml Network REQUIRED)

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
if(WIN32)
    target_link_libraries(FeedReader PRIVATE retroshare retroshare-gui)
endif()

target_link_libraries(FeedReader PRIVATE
    Qt5::Core
    Qt5::Widgets
    Qt5::Xml
    Qt5::Network
    CURL::libcurl
    LibXml2::LibXml2
    LibXslt::LibXslt
)

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
