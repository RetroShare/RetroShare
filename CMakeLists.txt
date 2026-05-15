cmake_minimum_required(VERSION 3.18)
project(RetroShare)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Versioning ---
find_package(Git REQUIRED)
execute_process(
    COMMAND ${GIT_EXECUTABLE} describe --tags --always --dirty
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE RS_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
message(STATUS "RetroShare Version: ${RS_VERSION}")

# --- Options (Ported from retroshare.pri) ---
option(RS_LIBRETROSHARE_STATIC "Build RetroShare static library" ON)
option(RS_LIBRETROSHARE_SHARED "Build RetroShare shared library" OFF)
option(RS_DEVELOPMENT_BUILD "Enable development build (no optimization)" ON)
option(RS_BITDHT "Enable BitDHT support" ON)
option(RS_SQLCIPHER "Enable SQLCipher support" ON)
option(RS_RNPLIB "Use RNP for PGP (recommended)" ON)
option(RS_JSON_API "Enable JSON API" OFF)
option(RS_WEBUI "Enable Web UI" OFF)
option(RS_USE_I2P_SAM3 "Enable I2P SAMv3 support" ON)
option(RS_GXS "Enable GXS (General eXchange System)" ON)
option(RS_GXSGUI      "Enable GXS services in GUI" ON)
option(RS_GXSCHANNELS "Enable GXS channels in GUI" ON)
option(RS_GXSFORUMS   "Enable GXS forums in GUI"   ON)
option(RS_GXSPOSTED   "Enable GXS posted in GUI"   ON)
option(RS_GXSCIRCLES  "Enable GXS circles in GUI"  ON)
option(RS_GXSWIKIPOS  "Enable GXS wiki"            OFF)
option(RS_GXSTHEWIRE  "Enable GXS wire"            OFF)
option(RS_GXSPHOTOSHARE "Enable GXS PhotoShare"    OFF)
option(RS_GXSIDENTITIES "Enable GXS identities in GUI" ON)
option(RS_BROADCAST_DISCOVERY "Enable broadcast discovery" ON)
option(RS_GUI "Build RetroShare GUI" ON)
option(RS_SERVICE "Build RetroShare Service" ON)
option(RS_PLUGINS "Enable plugins support" OFF)
option(RS_WARN_DEPRECATED "Show deprecated warnings" OFF)

option(RS_MESSENGER "Enable Messenger Window" ON)
option(RS_IDLE      "Enable Idle support"     ON)

if(RS_USE_I2P_SAM3)
    add_definitions(-DRS_USE_I2P_SAM3)
endif()

if(RS_GXSWIKIPOS)
    add_definitions(-DRS_USE_WIKI)
endif()

if(RS_GXSTHEWIRE)
    add_definitions(-DRS_USE_WIRE)
endif()

if(RS_GXSPHOTOSHARE)
    add_definitions(-DRS_USE_PHOTO)
endif()

# --- Dependencies ---
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

if(RS_GUI)
    find_package(Qt5 COMPONENTS Core Widgets Gui Network Xml PrintSupport Multimedia REQUIRED)
    find_package(Qt5 COMPONENTS LinguistTools QUIET)
    # Note: We can add Qt6 support later as in the .pro files
endif()

if(RS_SQLCIPHER)
    # We will need to find sqlcipher properly, for now fallback to standard sqlite if not found
    find_library(SQLCIPHER_LIBRARY NAMES sqlcipher)
    if(NOT SQLCIPHER_LIBRARY)
        message(WARNING "SQLCipher not found, falling back to SQLite3")
        find_package(SQLite3 REQUIRED)
    endif()
else()
    find_package(SQLite3 REQUIRED)
endif()

# --- Subdirectories ---
# 1. Support libs (Librnp, etc.)
if(RS_RNPLIB)
    add_subdirectory(supportlibs/librnp EXCLUDE_FROM_ALL)
endif()

if(RS_JSON_API)
    # Disable restbed tests/examples and build as static library
    set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(BUILD_SSL ON CACHE BOOL "" FORCE)
    set(BUILD_STATIC_LIBRARY ON CACHE BOOL "" FORCE)
    set(BUILD_SHARED_LIBRARY OFF CACHE BOOL "" FORCE)
    add_subdirectory(supportlibs/restbed EXCLUDE_FROM_ALL)
endif()

# 2. Core libs
add_subdirectory(libbitdht)
add_subdirectory(openpgpsdk)
add_subdirectory(libretroshare)

# 3. Executables
if(RS_SERVICE)
    add_subdirectory(retroshare-service)
endif()

if(RS_GUI)
    add_subdirectory(retroshare-gui)
endif()

if(RS_PLUGINS)
    add_subdirectory(plugins)
endif()
