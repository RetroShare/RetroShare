# RetroShare decentralized communication platform
#
# Copyright (C) 2021  Gioacchino Mazzurco <gio@eigenlab.org>
# Copyright (C) 2021  Asociación Civil Altermundi <info@altermundi.net>
#
# SPDX-License-Identifier: CC0-1.0

cmake_minimum_required (VERSION 3.18.0)
project(retroshare)

# sqlcipher
option(
	RS_SQLCIPHER
	"SQLCipher encryption for GXS database"
	ON )

# rs_gxs_send_all
option(
	RS_GXS_SEND_ALL
	"GXS distribute all available messages on request, indipendently from \
	local sync timer"
	ON )

# bitdht
option(
	RS_BITDHT
	"Use bitdht (BitTorrent DHT own implementation) to look for online peers"
	ON )

# use_dht_stunner
option(
	RS_BITDHT_STUNNER
	"Use bitdht (BitTorrent DHT own implementation) for NAT type discovery and \
	attempt the STUN (Session Traversal Utilities for NAT)"
	ON )

# use_dht_stunner_ext_ip
option(
	RS_BITDHT_STUNNER_EXT_IP
	"Use bitdht (BitTorrent DHT own implementation) stunner to figure out our \
	external IP. As this purely relying on random DHT peers that answer our \
	request, it can easily be abused. Therefore, it is turned off by default."
	OFF )

# rs_jsonapi
option(
	RS_JSON_API
	"Use restbed to expose libretroshare as JSON API via HTTP"
	OFF )

# rs_deep_forums_index
option(
	RS_FORUM_DEEP_INDEX
	"Xapian based full text index and search of GXS forums"
	OFF )

# rs_broadcast_discovery
option(
	RS_BRODCAST_DISCOVERY
	"Local area network peer discovery via udp-discovery-cpp"
	ON )

# rs_dh_init_check
option(
	RS_DH_PRIME_INIT_CHECK
	"Check Diffie Hellman prime at each startup. This is not necessary and on \
	all Android mobile phones tested this take at least one minute at startup \
	which is untolerable for most phone users."
	ON )

option(
	RS_MINIUPNPC
	"Forward ports in NAT router via miniupnpc"
	ON
)

include(CMakeDependentOption)
cmake_dependent_option(
	RS_LIBUPNP
	"Forward ports in NAT router via libupnp (unstable)"
	OFF
	"NOT RS_MINIUPNPC"
	OFF
)

option(
	RS_LIBRETROSHARE_STATIC
	"Build RetroShare static library"
	ON
)

cmake_dependent_option(
	RS_LIBRETROSHARE_SHARED
	"Build RetroShare shared library"
	OFF
	"NOT RS_LIBRETROSHARE_STATIC"
	OFF
)

# rs_deprecatedwarning
option(
	RS_WARN_DEPRECATED
	"Print warning about RetroShare deprecated components usage during build"
	ON
)

# rs_cppwarning
option(
	RS_WARN_LESS
	"Silence a few at the moment very common warnings about RetroShare \
	components during build"
	OFF
)

# rs_v07_changes
option(
	RS_V07_BREAKING_CHANGES
	"Enable retro-compatibility breaking changes planned for RetroShare 0.7.0"
	OFF
)

set(
	RS_DATA_DIR
	"${CMAKE_INSTALL_PREFIX}/share/retroshare"
	CACHE STRING
	"Path where to install RetroShare system wide data" )

################################################################################

include(src/CMakeLists.txt)
list(TRANSFORM RS_SOURCES PREPEND src/)

if(RS_LIBRETROSHARE_STATIC)
	add_library(${PROJECT_NAME} STATIC ${RS_SOURCES})
endif(RS_LIBRETROSHARE_STATIC)

if(RS_LIBRETROSHARE_SHARED)
	add_library(${PROJECT_NAME} SHARED ${RS_SOURCES})

	## Ensure statically linked libraries such as openpgpsdk are compiled with
	## PIC Which is needed for shared library
	set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif(RS_LIBRETROSHARE_SHARED)

target_include_directories(
	${PROJECT_NAME}
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src )


find_package(OpenSSL REQUIRED)
target_include_directories(${PROJECT_NAME} PRIVATE ${OPENSSL_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE OpenSSL::SSL OpenSSL::Crypto)

################################################################################

add_subdirectory(../openpgpsdk ${CMAKE_BINARY_DIR}/openpgpsdk)
target_link_libraries(${PROJECT_NAME} PRIVATE openpgpsdk)

if(RS_BITDHT)
	add_compile_definitions(RS_USE_BITDHT)
	add_subdirectory(../libbitdht ${CMAKE_BINARY_DIR}/libbitdht)
	target_link_libraries(${PROJECT_NAME} PRIVATE libbitdht)
endif(RS_BITDHT)

## TODO: Check if https://github.com/rbock/sqlpp11 or
## https://github.com/rbock/sqlpp17 may improve GXS code
if(RS_SQLCIPHER)
	find_library(RS_SQL_LIB "sqlcipher" REQUIRED)
	find_path(
		RS_SQL_LIB_INCLUDE "sqlcipher/sqlite3.h"
		PATH_SUFFIXES "include" "includes"
		REQUIRED )
	target_include_directories(
		${PROJECT_NAME}
		PRIVATE "${RS_SQL_LIB_INCLUDE}/sqlcipher" )
	target_link_libraries(${PROJECT_NAME} PRIVATE ${RS_SQL_LIB})
else()
	add_compile_definitions(NO_SQLCIPHER)
	find_package(SQLite3 REQUIRED)
	target_link_libraries(${PROJECT_NAME} PRIVATE SQLite::SQLite3)
endif()

add_compile_definitions(
	SQLITE_HAS_CODEC
	RS_ENABLE_GXS
	GXS_ENABLE_SYNC_MSGS
	RS_USE_GXS_DISTANT_SYNC
	RS_GXS_TRANS
	V07_NON_BACKWARD_COMPATIBLE_CHANGE_001
	V07_NON_BACKWARD_COMPATIBLE_CHANGE_002
	V07_NON_BACKWARD_COMPATIBLE_CHANGE_003 )

if(RS_V07_BREAKING_CHANGES)
	add_compile_definitions(
		V07_NON_BACKWARD_COMPATIBLE_CHANGE_004
		V07_NON_BACKWARD_COMPATIBLE_CHANGE_UNNAMED )
endif()

if(RS_MINIUPNPC)
	add_compile_definitions(RS_USE_LIBMINIUPNPC)
endif(RS_MINIUPNPC)

if(RS_LIBUPNP)
	add_compile_definitions(RS_USE_LIBUPNP)
endif(RS_LIBUPNP)

if(RS_GXS_SEND_ALL)
	add_compile_definitions(RS_GXS_SEND_ALL)
endif(RS_GXS_SEND_ALL)

if(RS_BRODCAST_DISCOVERY)
	add_subdirectory(
	../supportlibs/udp-discovery-cpp/
	${CMAKE_BINARY_DIR}/supportlibs/udp-discovery-cpp/ )
	target_link_libraries(${PROJECT_NAME} PRIVATE udp-discovery)

	## Temporary work around target_include_directories should be added upstream
	include_directories(../supportlibs/udp-discovery-cpp/)
endif(RS_BRODCAST_DISCOVERY)

if(NOT RS_WARN_DEPRECATED)
	add_compile_definitions(RS_NO_WARN_DEPRECATED)
	target_compile_options(
		${PROJECT_NAME} PRIVATE
		-Wno-deprecated -Wno-deprecated-declarations )
endif(RS_WARN_DEPRECATED)

if(RS_WARN_LESS)
	add_compile_definitions(RS_NO_WARN_CPP)
	
	target_compile_options(
		${PROJECT_NAME} PRIVATE
		-Wno-cpp -Wno-inconsistent-missing-override )
endif(RS_WARN_LESS)

################################################################################

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	add_compile_definitions(RS_DATA_DIR="${RS_DATA_DIR}")
endif(CMAKE_SYSTEM_NAME STREQUAL "Linux")

## Useful to debug CMake
#set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
