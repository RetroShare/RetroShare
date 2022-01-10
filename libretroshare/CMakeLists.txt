# RetroShare decentralized communication platform
#
# Copyright (C) 2021  Gioacchino Mazzurco <gio@eigenlab.org>
# Copyright (C) 2021  Asociación Civil Altermundi <info@altermundi.net>
#
# SPDX-License-Identifier: CC0-1.0

cmake_minimum_required (VERSION 3.18.0)
project(retroshare)

include(CMakeDependentOption)

set(FETCHCONTENT_QUIET OFF)
include(FetchContent)

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
cmake_dependent_option(
	RS_BITDHT_STUNNER
	"Use bitdht (BitTorrent DHT own implementation) for NAT type discovery and \
	attempt the STUN (Session Traversal Utilities for NAT)"
	ON
	"RS_BITDHT"
	OFF )

# use_dht_stunner_ext_ip
cmake_dependent_option(
	RS_BITDHT_STUNNER_EXT_IP
	"Use bitdht (BitTorrent DHT own implementation) stunner to figure out our \
	external IP. As this purely relying on random DHT peers that answer our \
	request, it can easily be abused. Therefore, it is turned off by default."
	OFF
	"RS_BITDHT_STUNNER"
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
	ON )

cmake_dependent_option(
	RS_LIBUPNP
	"Forward ports in NAT router via libupnp (unstable)"
	OFF
	"NOT RS_MINIUPNPC"
	OFF )

option(
	RS_LIBRETROSHARE_STATIC
	"Build RetroShare static library"
	ON )

cmake_dependent_option(
	RS_LIBRETROSHARE_SHARED
	"Build RetroShare shared library"
	OFF
	"NOT RS_LIBRETROSHARE_STATIC"
	OFF )

# rs_deprecatedwarning
option(
	RS_WARN_DEPRECATED
	"Print warning about RetroShare deprecated components usage during build"
	ON )

# rs_cppwarning
option(
	RS_WARN_LESS
	"Silence a few at the moment very common warnings about RetroShare \
	components during build"
	OFF )

# rs_v07_changes
option(
	RS_V07_BREAKING_CHANGES
	"Enable retro-compatibility breaking changes planned for RetroShare 0.7.0"
	OFF )

set(
	RS_DATA_DIR
	"${CMAKE_INSTALL_PREFIX}/share/retroshare"
	CACHE STRING
	"Path where to install RetroShare system wide data" )

option(
	RS_EXPORT_JNI_ONLOAD
	"Export libretroshare JNI_OnLoad. See src/rs_android/rsjni.cpp for details"
	ON )

################################################################################

find_package(Git REQUIRED)

#function(check_submodule sPath)
#	if(NOT EXISTS "${sPath}/.git" )
#		message("Initializing submodule ${sPath}")
#		execute_process(
#			COMMAND "${GIT_EXECUTABLE}" submodule update --init
#			WORKING_DIRECTORY "${sPath}"
#			COMMAND_ECHO STDERR
#			COMMAND_ERROR_IS_FATAL ANY)
#	endif()
#endfunction()

################################################################################

include(src/CMakeLists.txt)
list(TRANSFORM RS_SOURCES PREPEND src/)
list(TRANSFORM RS_PUBLIC_HEADERS PREPEND src/)

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

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

# TODO: install public headers

find_package(OpenSSL REQUIRED)
target_include_directories(${PROJECT_NAME} PRIVATE ${OPENSSL_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE OpenSSL::SSL OpenSSL::Crypto)

################################################################################

set(OPENPGPSDK_DEVEL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../openpgpsdk/")
if(EXISTS "${OPENPGPSDK_DEVEL_DIR}/.git" )
	message(
		CHECK_PASS
		"openpgpsdk submodule found at ${OPENPGPSDK_DEVEL_DIR} using it" )
	add_subdirectory(${OPENPGPSDK_DEVEL_DIR} ${CMAKE_BINARY_DIR}/openpgpsdk)
else()
	FetchContent_Declare(
		openpgpsdk
		GIT_REPOSITORY "https://gitlab.com/RetroShare/openpgpsdk.git"
		GIT_TAG "origin/master"
		GIT_SHALLOW TRUE
		GIT_PROGRESS TRUE
		TIMEOUT 10
	)
	FetchContent_MakeAvailable(openpgpsdk)
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE openpgpsdk)

################################################################################

if(RS_BITDHT)
	set(BITDHT_DEVEL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../libbitdht/")
	if(EXISTS "${BITDHT_DEVEL_DIR}/.git" )
		message(
			CHECK_PASS
			"BitDHT submodule found at ${BITDHT_DEVEL_DIR} using it" )
		add_subdirectory(${BITDHT_DEVEL_DIR} ${CMAKE_BINARY_DIR}/bitdht)
		set(RS_BITDHT_DIR "${BITDHT_DEVEL_DIR}")
	else()
		FetchContent_Declare(
			bitdht
			GIT_REPOSITORY "https://gitlab.com/RetroShare/bitdht.git"
			GIT_TAG "origin/master"
			GIT_SHALLOW TRUE
			GIT_PROGRESS TRUE
			TIMEOUT 10
		)
		FetchContent_MakeAvailable(bitdht)

		set(RS_BITDHT_DIR "${bitdht_SOURCE_DIR}")
	endif()

	add_compile_definitions(RS_USE_BITDHT)
	target_link_libraries(${PROJECT_NAME} PRIVATE bitdht)
	
	if(RS_BITDHT_STUNNER)
		add_compile_definitions(RS_USE_DHT_STUNNER)

		if(RS_BITDHT_STUNNER_EXT_IP)
			# TODO: Refactor this define to use proper naming
			add_compile_definitions(ALLOW_DHT_STUNNER)
		endif(RS_BITDHT_STUNNER_EXT_IP)
	endif(RS_BITDHT_STUNNER)
endif(RS_BITDHT)

################################################################################

if(RS_JSON_API)
	find_package(Doxygen REQUIRED)
	find_package(Python3 REQUIRED)

	set(
		JSON_API_GENERATOR_WORK_DIR
		"${CMAKE_BINARY_DIR}/jsonapi-generator.workdir/" )

	set(
		JSON_API_GENERATOR_DOXYFILE
		"${JSON_API_GENERATOR_WORK_DIR}/jsonapi-generator-doxygen.conf" )

	set(
		JSONAPI_GENERATOR_OUTPUT_DIR
		"${JSON_API_GENERATOR_WORK_DIR}/src/" )

	set(
		JSONAPI_GENERATOR_SOURCE_DIR
		"${CMAKE_CURRENT_SOURCE_DIR}/src/jsonapi/" )
		
	set(
		JSONAPI_GENERATOR_EXECUTABLE
		"${JSONAPI_GENERATOR_SOURCE_DIR}/jsonapi-generator.py" )

	file(
		COPY "src/jsonapi/jsonapi-generator-doxygen.conf"
		DESTINATION "${JSON_API_GENERATOR_WORK_DIR}" )

	file(
		APPEND
		"${JSON_API_GENERATOR_DOXYFILE}"
		"OUTPUT_DIRECTORY=${JSONAPI_GENERATOR_OUTPUT_DIR}\n"
		"INPUT=${CMAKE_CURRENT_SOURCE_DIR}" )

	add_custom_command(
		OUTPUT
			"${JSONAPI_GENERATOR_OUTPUT_DIR}/jsonapi-includes.inl"
			"${JSONAPI_GENERATOR_OUTPUT_DIR}/jsonapi-wrappers.inl"
		COMMAND ${DOXYGEN_EXECUTABLE} ${JSON_API_GENERATOR_DOXYFILE}
		COMMAND
			${Python3_EXECUTABLE} ${JSONAPI_GENERATOR_EXECUTABLE}
			${JSONAPI_GENERATOR_SOURCE_DIR} ${JSONAPI_GENERATOR_OUTPUT_DIR}
		MAIN_DEPENDENCY "${JSONAPI_GENERATOR_EXECUTABLE}"
		DEPENDS ${JSON_API_GENERATOR_DOXYFILE} ${RS_PUBLIC_HEADERS} )

	target_sources(
		${PROJECT_NAME} PRIVATE
		"${JSONAPI_GENERATOR_OUTPUT_DIR}/jsonapi-includes.inl"
		"${JSONAPI_GENERATOR_OUTPUT_DIR}/jsonapi-wrappers.inl" )

	include_directories(${JSONAPI_GENERATOR_OUTPUT_DIR})

	set(BUILD_TESTS OFF CACHE BOOL "Do not build restbed tests")
	set(BUILD_SSL OFF CACHE BOOL "Do not build restbed SSL support")

	FetchContent_Declare(
		restbed
		GIT_REPOSITORY "https://github.com/Corvusoft/restbed.git"
		GIT_TAG "4.8"
		GIT_SUBMODULES dependency/asio dependency/catch
		GIT_SHALLOW TRUE
		GIT_PROGRESS TRUE
		TIMEOUT 10
	)
	FetchContent_MakeAvailable(restbed)

	target_link_libraries(${PROJECT_NAME} PRIVATE restbed)

	## TODO: work around target_include_directories should be added upstream
	include_directories(${restbed_SOURCE_DIR}/source/)

	add_compile_definitions(RS_JSONAPI)
endif(RS_JSON_API)

################################################################################

if(RS_FORUM_DEEP_INDEX)
	find_package(Xapian REQUIRED)
	target_link_libraries(${PROJECT_NAME} PRIVATE ${XAPIAN_LIBRARIES})

	add_compile_definitions(RS_DEEP_FORUMS_INDEX)
endif(RS_FORUM_DEEP_INDEX)

################################################################################

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

if(RS_DH_PRIME_INIT_CHECK)
	add_compile_definitions(RS_DISABLE_DIFFIE_HELLMAN_INIT_CHECK)
endif(RS_DH_PRIME_INIT_CHECK)

if(RS_MINIUPNPC)
	add_compile_definitions(RS_USE_LIBMINIUPNPC)
endif(RS_MINIUPNPC)

if(RS_LIBUPNP)
	message(FATAL_ERROR "UPnP support via libupnp is currently not supported")
	#add_compile_definitions(RS_USE_LIBUPNP)
endif(RS_LIBUPNP)

if(RS_GXS_SEND_ALL)
	add_compile_definitions(RS_GXS_SEND_ALL)
endif(RS_GXS_SEND_ALL)

if(RS_BRODCAST_DISCOVERY)
	## TODO: upstream option to disable tests building
	set(BUILD_EXAMPLE OFF CACHE BOOL "Do not build udp-discovery-cpp examples")
	set(BUILD_TOOL OFF CACHE BOOL "Do not build udp-discovery-tool application")
	FetchContent_Declare(
		udp-discovery-cpp
		GIT_REPOSITORY "https://github.com/truvorskameikin/udp-discovery-cpp.git"
		GIT_TAG "origin/master"
		GIT_SHALLOW TRUE
		GIT_PROGRESS TRUE
		TIMEOUT 10
	)
	FetchContent_MakeAvailable(udp-discovery-cpp)

	target_link_libraries(${PROJECT_NAME} PRIVATE udp-discovery)

	## TODO: Temporary work around target_include_directories should be added
	## upstream
	include_directories(${udp-discovery-cpp_SOURCE_DIR})
endif(RS_BRODCAST_DISCOVERY)

if(NOT RS_WARN_DEPRECATED)
	add_compile_definitions(RS_NO_WARN_DEPRECATED)
	target_compile_options(
		${PROJECT_NAME} PRIVATE
		-Wno-deprecated -Wno-deprecated-declarations )
endif(NOT RS_WARN_DEPRECATED)

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

################################################################################

if(RS_EXPORT_JNI_ONLOAD)
	add_compile_definitions(RS_LIBRETROSHARE_EXPORT_JNI_ONLOAD)
endif(RS_EXPORT_JNI_ONLOAD)

################################################################################

#if(CMAKE_SYSTEM_NAME STREQUAL "Android")
if(RS_ANDROID)
	FetchContent_Declare(
		jni-hpp
		GIT_REPOSITORY "https://gitlab.com/RetroShare/jni-hpp.git"
		GIT_TAG "origin/master"
		GIT_SHALLOW TRUE
		GIT_PROGRESS TRUE
		TIMEOUT 10
	)
	FetchContent_MakeAvailable(jni-hpp)

	include_directories(${jni-hpp_SOURCE_DIR}/include)

	if(RS_BITDHT)
		set(RS_ANDROID_ASSETS_DIR "${CMAKE_BINARY_DIR}/android-assets")
		set(RS_ANDROID_VALUES_DIR "${RS_ANDROID_ASSETS_DIR}/values")
		file(MAKE_DIRECTORY "${RS_ANDROID_VALUES_DIR}")
		file(
			COPY "${RS_BITDHT_DIR}/src/bitdht/bdboot.txt"
			DESTINATION "${RS_ANDROID_VALUES_DIR}" )
		set(
			ANDROID_ASSETS_DIRECTORIES
			"${RS_ANDROID_ASSETS_DIR};${ANDROID_ASSETS_DIRECTORIES}" )
	endif(RS_BITDHT)
endif(RS_ANDROID)
#endif(CMAKE_SYSTEM_NAME STREQUAL "Android")

################################################################################

## Useful to debug CMake
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
