################################################################################
# retroshare-gui/CMakeLists.txt
################################################################################

# project() and common options are inherited from the root CMakeLists.txt

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# --- Qt Dependencies ---
# Root already found Qt5, we just use it here
find_package(Qt5 COMPONENTS Core Widgets Xml Network Multimedia PrintSupport REQUIRED)
if(UNIX AND NOT APPLE)
    find_package(Qt5 COMPONENTS X11Extras REQUIRED)
endif()
find_package(Qt5 COMPONENTS LinguistTools QUIET)
set(RS_LINK_LIBRARIES Qt5::Core Qt5::Widgets Qt5::Xml Qt5::Network Qt5::Multimedia Qt5::PrintSupport)
if(UNIX AND NOT APPLE)
    list(APPEND RS_LINK_LIBRARIES Qt5::X11Extras)
endif()

# --- Sources ---
include(src/CMakeLists.txt)

# --- Translations (.ts -> .qm) ---
file(GLOB TS_FILES "src/lang/*.ts")
if(Qt5LinguistTools_FOUND)
    qt5_add_translation(QM_FILES ${TS_FILES})
else()
    message(STATUS "Qt5LinguistTools not found, trying to use existing .qm files or lrelease")
    find_program(LRELEASE_EXECUTABLE NAMES lrelease-qt5 lrelease)
    foreach(ts_file ${TS_FILES})
        get_filename_component(ts_name ${ts_file} NAME_WE)
        set(qm_src_file "${CMAKE_CURRENT_SOURCE_DIR}/src/lang/${ts_name}.qm")
        set(qm_bin_file "${CMAKE_CURRENT_BINARY_DIR}/${ts_name}.qm")
        
        if(EXISTS ${qm_src_file})
            # Use existing .qm file from source tree
            list(APPEND QM_FILES ${qm_src_file})
        elseif(LRELEASE_EXECUTABLE)
            # Try to generate it, but don't fail if lrelease is broken
            add_custom_command(
                OUTPUT ${qm_bin_file}
                COMMAND ${LRELEASE_EXECUTABLE} ${ts_file} -qm ${qm_bin_file} || ${CMAKE_COMMAND} -E echo "Warning: lrelease failed for ${ts_name}"
                COMMAND ${CMAKE_COMMAND} -E touch ${qm_bin_file}
                DEPENDS ${ts_file}
                VERBATIM
            )
            list(APPEND QM_FILES ${qm_bin_file})
        endif()
    endforeach()
endif()

# --- Platform Resources ---
if(WIN32)
    set(RS_GUI_RESOURCES ${RS_GUI_RESOURCES} gui/images/retroshare_win.rc)
endif()

# --- Executable ---
add_executable(retroshare-gui 
    ${RS_GUI_SOURCES} 
    ${RS_GUI_HEADERS}
    ${RS_GUI_FORMS} 
    ${RS_GUI_QTRESOURCES}
    ${QM_FILES}
    ${RS_GUI_RESOURCES}
)

set_target_properties(retroshare-gui PROPERTIES 
    OUTPUT_NAME retroshare 
    AUTOUIC_SEARCH_PATHS "${CMAKE_CURRENT_SOURCE_DIR}/src/gui/gxs;${CMAKE_CURRENT_SOURCE_DIR}/src/gui/common;${CMAKE_CURRENT_SOURCE_DIR}/src/gui/settings;${CMAKE_CURRENT_SOURCE_DIR}/src/gui/chat;${CMAKE_CURRENT_SOURCE_DIR}/src/gui/msgs"
    ENABLE_EXPORTS ON
)

# --- Linkage ---
target_link_libraries(retroshare-gui PRIVATE retroshare ${RS_LINK_LIBRARIES})

# --- Includes ---
target_include_directories(retroshare-gui PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/
    ${CMAKE_CURRENT_SOURCE_DIR}/src/retroshare-gui
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gui
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gui/common
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gui/gxs
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gui/settings
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gui/chat
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gui/connect
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gui/groups
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gui/msgs
    ${CMAKE_SOURCE_DIR}/supportlibs/pegmarkdown
    ${CMAKE_SOURCE_DIR}/supportlibs/libsam3/src/libsam3
    ${CMAKE_BINARY_DIR}
)

if(RS_JSON_API)
    target_include_directories(retroshare-gui PRIVATE ${CMAKE_SOURCE_DIR}/supportlibs/restbed/source/)
endif()

# --- Compile Definitions ---
target_compile_definitions(retroshare-gui PRIVATE 
    RS_RELEASE_VERSION="${RS_VERSION}" 
    RS_MAJOR_VERSION=${RS_MAJOR_VERSION}
    RS_MINOR_VERSION=${RS_MINOR_VERSION}
    RS_MINI_VERSION=${RS_MINI_VERSION}
    RS_EXTRA_VERSION="${RS_EXTRA_VERSION}"
    RS_LIB_VERSION_HASH="${RS_LIB_VERSION}"
    TARGET="retroshare"
    RS_DIRECT_CHAT
)


if(UNIX AND NOT APPLE)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(X11 REQUIRED x11)
    pkg_check_modules(XSCRNSAVER REQUIRED xscrnsaver)
    
    target_link_libraries(retroshare-gui PRIVATE ${X11_LIBRARIES} ${XSCRNSAVER_LIBRARIES})
    target_compile_definitions(retroshare-gui PRIVATE HAVE_XSS)
endif()

# --- Installation ---
install(TARGETS retroshare-gui RUNTIME DESTINATION bin)
