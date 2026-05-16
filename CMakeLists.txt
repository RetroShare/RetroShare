cmake_minimum_required(VERSION 3.18)
project(RetroShare)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Global silence for deprecated warnings during migration
add_compile_options(-Wno-deprecated-declarations -Wno-cpp)

# --- Versioning ---
find_package(Git REQUIRED)

# 1. Get Root Version
execute_process(
    COMMAND ${GIT_EXECUTABLE} describe --tags --always --dirty
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE RS_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
message(STATUS "RetroShare Version: ${RS_VERSION}")

# Parse Root Version (similar to libretroshare's logic but for root)
set(V_GIT_DESCRIBE_REGEXP "^v([0-9]+).([0-9]+).([0-9]+)(.*)$")
if(RS_VERSION MATCHES "${V_GIT_DESCRIBE_REGEXP}")
    string(REGEX REPLACE "${V_GIT_DESCRIBE_REGEXP}" "\\1;\\2;\\3;\\4" V_GIT_DESCRIBE_LIST ${RS_VERSION})
    list(GET V_GIT_DESCRIBE_LIST 0 MAJOR_V)
    list(GET V_GIT_DESCRIBE_LIST 1 MINOR_V)
    list(GET V_GIT_DESCRIBE_LIST 2 MINI_V)
    list(GET V_GIT_DESCRIBE_LIST 3 EXTRA_V)
    
    set(RS_MAJOR_VERSION "${MAJOR_V}" CACHE STRING "Major version" FORCE)
    set(RS_MINOR_VERSION "${MINOR_V}" CACHE STRING "Minor version" FORCE)
    set(RS_MINI_VERSION  "${MINI_V}"  CACHE STRING "Mini version"  FORCE)
    set(RS_EXTRA_VERSION "${EXTRA_V}" CACHE STRING "Extra version" FORCE)
else()
    # Fallback if tag format is different
    set(RS_MAJOR_VERSION "0" CACHE STRING "Major version" FORCE)
    set(RS_MINOR_VERSION "6" CACHE STRING "Minor version" FORCE)
    set(RS_MINI_VERSION  "7" CACHE STRING "Mini version"  FORCE)
    set(RS_EXTRA_VERSION "a-unknown" CACHE STRING "Extra version" FORCE)
endif()

# 2. Get LibRetroShare Version (for RS_LIB_VERSION_HASH)
execute_process(
    COMMAND ${GIT_EXECUTABLE} describe --tags --always --dirty
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/libretroshare"
    OUTPUT_VARIABLE RS_LIB_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)
if(NOT RS_LIB_VERSION)
    set(RS_LIB_VERSION "[version not available]")
endif()
message(STATUS "libretroshare Version: ${RS_LIB_VERSION}")


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

if(RS_USE_I2P_SAM3)
    add_subdirectory(supportlibs/libsam3 EXCLUDE_FROM_ALL)
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
