################################################################################
# retroshare-gui/CMakeLists.txt                                                #
# This file is part of Retroshare-GUI.                                         #
#                                                                              #
# Copyright (C) 2022      Retroshare team <retroshare.team@gmailcom>           #
# Copyright (C) 2026      David Bears <dbear4q@gmail.com>                      #
#                                                                              #
# This program is free software: you can redistribute it and/or modify         #
# it under the terms of the GNU Affero General Public License as               #
# published by the Free Software Foundation, either version 3 of the           #
# License, or (at your option) any later version.                              #
#                                                                              #
# This program is distributed in the hope that it will be useful,              #
# but WITHOUT ANY WARRANTY; without even the implied warranty of               #
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                #
# GNU Affero General Public License for more details.                          #
#                                                                              #
# You should have received a copy of the GNU Affero General Public License     #
# along with this program.  If not, see <https://www.gnu.org/licenses/>.       #
################################################################################

# 3.1 : configure_package_config_file(INSTALL_PREFIX)
# 3.4 : CPACK_VERBATIM_VARIABLES
# 3.7 : if(VERSION_GREATER_EQUAL)
# 3.10: string(PREPEND)
# 3.11: add_executable() ... target_sources()
# 3.11: configure_package_config_file(COMPATIBILITY SameMinorVersion)
cmake_minimum_required(VERSION 3.11...4.4)

if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.24)
	list(PREPEND CMAKE_PROJECT_TOP_LEVEL_INCLUDES
		"${CMAKE_CURRENT_SOURCE_DIR}/mk/cmake/fetch_provider.cmake"
	)
else()
	include(mk/cmake/fetch_provider.cmake)
endif()

project(retroshare-gui
	VERSION 0.6.7.3
	LANGUAGES CXX C
)

set(PROJECT_NAME_PRETTY "RetroShare-GUI")
string(TOLOWER ${PROJECT_NAME} PROJECT_NAME_LOWER)


################################################################################
### options

include(GNUInstallDirs)
include(CMakeDependentOption)

set(RS_VERSION_SUFFIX
	""
	CACHE STRING
	"Custom version suffix; useful for downstream packaging."
)

set(
	RS_INSTALL_DATADIR
	"${CMAKE_INSTALL_DATADIR}/retroshare"
	CACHE PATH
	"Path where to install RetroShare system wide data"
)

option(
	RS_DEVELOPMENT_BUILD
	"Disable optimization to speed up build, enable verbose build log. \
		just for development purposes, not suitable for library usage"
	ON
)

option(
	RS_JSON_API
	"Use restbed to expose libretroshare as JSON API via HTTP"
	ON
)

# Mirrors the option of the same name in libretroshare/CMakeLists.txt.
option(
	RS_WARN_DEPRECATED
	"Print warnings about deprecated RetroShare components usage during build"
	OFF
)

option(
	RS_USE_NATIVE_DIALOGS
	"Use the operating system native file/font/directory dialogs. When OFF, Qt's \
		own (non-native) dialogs are used instead \
		(QFileDialog::DontUseNativeDialog)."
	OFF
)

option(
	RS_SERVICE_TERMINAL_LOGIN
	"Enable RetroShare login via terminal"
	ON
)

option(RS_GXSGUI      "Enable GXS services in GUI" ON )
option(RS_GXSCHANNELS "Enable GXS channels in GUI" ON )
option(RS_GXSFORUMS   "Enable GXS forums in GUI"   ON )
option(RS_GXSPOSTED   "Enable GXS posted in GUI"   ON )
option(RS_GXSCIRCLES  "Enable GXS circles in GUI"  ON )
option(RS_GXSTHEWIRE  "Enable GXS The Wire in GUI" OFF)
option(RS_GXSWIKIPOS  "Enable GXS Wiki in GUI"     OFF)
option(RS_GUI_CMARK    "Enable CommonMark support in GUI"  OFF )
set(RS_GXSIDENTITIES ON CACHE BOOL "Enable GXS identities in GUI" FORCE)
set(RS_IDLE ON CACHE BOOL "Enable Idle support" FORCE)

cmake_dependent_option(
	RS_SERVICE_TERMINAL_WEBUI_PASSWORD
	"Enable settin Web UI password via terminal in retroshare-service"
	OFF
	"RS_WEBUI"
	OFF
)

cmake_dependent_option(
	RS_WEBUI
	"Install RetroShare Web UI"
	OFF
	"RS_JSON_API"
	OFF
)

################################################################################
### dependencies

list(APPEND CMAKE_MODULE_PATH
	${PROJECT_SOURCE_DIR}/mk/cmake
	${PROJECT_SOURCE_DIR}/mk/cmake/fetch_provider_packages
)
set(CMAKE_FIND_PACKAGE_PREFER_CONFIG TRUE)

find_package(RetroShare REQUIRED)


################################################################################
### version

if(NOT PROJECT_VERSION)
	message(SEND_ERROR "${PROJECT_NAME_PRETTY} version seems to be missing!")
endif()
if(DEFINED RS_VERSION_SUFFIX AND NOT (RS_VERSION_SUFFIX STREQUAL ""))
	set(PROJECT_VERSION_FULL "${PROJECT_VERSION}-${RS_VERSION_SUFFIX}")
else()
	include(GetGitRevisionDescription)
	git_describe_working_tree(PROJECT_VERSION_FULL --dirty --match "v*.*.*")
	if(PROJECT_VERSION_FULL) # AND EXISTS ${PROJECT_SOURCE_DIR}/.git
		string(SUBSTRING ${PROJECT_VERSION_FULL} 1 -1 PROJECT_VERSION_FULL)
		string(FIND ${PROJECT_VERSION_FULL}- ${PROJECT_VERSION}- pos)
		if(NOT pos EQUAL 0)
			message(WARNING "${PROJECT_NAME_PRETTY} version mismatch!"
				" Declared version is ${PROJECT_VERSION},"
				" but git describe returned ${PROJECT_VERSION_FULL}."
			)
			string(PREPEND PROJECT_VERSION *)
			string(PREPEND PROJECT_VERSION_FULL *)
		endif()
	else()
		set(PROJECT_VERSION_FULL "${PROJECT_VERSION}-unknown")
	endif()
endif()
message(STATUS "${PROJECT_NAME_PRETTY} version: ${PROJECT_VERSION_FULL}")

################################################################################
### general build options

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(RS_DEVELOPMENT_BUILD)
	set(CMAKE_VERBOSE_MAKEFILE ON)
	set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif(RS_DEVELOPMENT_BUILD)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

################################################################################
### targets

add_executable(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME} PUBLIC retroshare::retroshare)

set_target_properties(${PROJECT_NAME} PROPERTIES
	RUNTIME_OUTPUT_NAME "retroshare-gui"
	ENABLE_EXPORTS TRUE
)

# NOTE: TARGET is NOT the executable name: it is the single-instance IPC socket
# name used in main.cpp / rshare.cpp (connectToServer(TARGET)). Kept as
# "retroshare" so the single-instance lock/socket name stays unchanged.
target_compile_definitions(${PROJECT_NAME}
	PRIVATE
	"RS_GUI_VERSION=\"${PROJECT_VERSION_FULL}\""
	RS_RELEASE_VERSION
	TARGET=\"retroshare\"
	RS_DIRECT_CHAT
)

if(RS_USE_NATIVE_DIALOGS)
	target_compile_definitions(${PROJECT_NAME} PRIVATE RS_NATIVEDIALOGS)
endif()

if(RS_DEVELOPMENT_BUILD)
	target_compile_options(${PROJECT_NAME} PRIVATE "-O0")
endif()

if(RS_SANITIZE)
	target_link_libraries(${PROJECT_NAME} PRIVATE asan ubsan)
endif()

if(NOT RS_WARN_DEPRECATED)
	target_compile_options(${PROJECT_NAME}
		PRIVATE
		-Wno-deprecated
		-Wno-deprecated-declarations
	)
	target_compile_definitions(${PROJECT_NAME} PUBLIC RS_NO_WARN_DEPRECATED)
endif()

################################################################################
### feature enablement

if(RS_JSON_API)
	target_compile_definitions(${PROJECT_NAME} PUBLIC RS_JSONAPI)
endif()

if(RS_WEBUI)
	target_compile_definitions(${PROJECT_NAME} PRIVATE RS_WEBUI)
endif()

if(RS_GXSCIRCLES)
	target_compile_definitions(${PROJECT_NAME} PRIVATE RS_USE_CIRCLES )
endif(RS_GXSCIRCLES)

################################################################################
### QT

# Detect Qt6 first, fall back to Qt5. All Qt usage below is version-agnostic via
# the ${QT_VERSION_MAJOR} variable so the same tree builds against both.
#
# Do NOT use find_package(QT NAMES Qt6 Qt5 ...) here: on distros that ship Qt5
# and Qt6 side by side in the same prefix (Debian/Ubuntu multiarch,
# /usr/lib/<arch>/cmake), the NAMES order is NOT honoured. CMake globs the
# sibling Qt5*/Qt6* config dirs and stops at whichever <name>Config.cmake it
# reaches first; that order is filesystem-dependent and routinely picks Qt5 even
# though Qt6 is listed first. We therefore probe Qt6 explicitly and fall back to
# Qt5. Bonus: this makes -DCMAKE_DISABLE_FIND_PACKAGE_Qt6=ON work as a reliable
# way to force a Qt5 build (it didn't, with the versionless QT package name).
if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.22)
	find_package( Qt6 QUIET COMPONENTS Core )
endif()
if( Qt6_FOUND )
	set( QT_VERSION_MAJOR 6 )
else()
	find_package( Qt5 COMPONENTS Core REQUIRED )
	set( QT_VERSION_MAJOR 5 )
endif()
find_package( Qt${QT_VERSION_MAJOR} COMPONENTS
	Core Widgets Xml Network Multimedia PrintSupport REQUIRED)
if(QT_VERSION_MAJOR EQUAL 6)
	# Provides QRegExp and other Qt5-era APIs still used by the GUI.
	find_package( Qt6 COMPONENTS Core5Compat REQUIRED)
endif()
if(APPLE)
	find_package( Qt${QT_VERSION_MAJOR} COMPONENTS Svg REQUIRED)
endif()
if(UNIX AND NOT APPLE AND QT_VERSION_MAJOR EQUAL 5)
	# QtX11Extras does not exist in Qt6: idle_platform.cpp uses QNativeInterface
	# (QX11Application) there instead, and X11/Xss are linked directly below.
	find_package( Qt5 COMPONENTS X11Extras REQUIRED )
endif()

target_link_libraries(${PROJECT_NAME}
	PRIVATE
	Qt${QT_VERSION_MAJOR}::Multimedia
	Qt${QT_VERSION_MAJOR}::Network
	Qt${QT_VERSION_MAJOR}::PrintSupport
	Qt${QT_VERSION_MAJOR}::Widgets
	Qt${QT_VERSION_MAJOR}::Xml
)
if(QT_VERSION_MAJOR EQUAL 6)
	target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Core5Compat)
endif()
if(APPLE)
	target_link_libraries(${PROJECT_NAME} PRIVATE Qt${QT_VERSION_MAJOR}::Svg)
endif()
if(UNIX AND NOT APPLE AND QT_VERSION_MAJOR EQUAL 5)
	target_link_libraries(${PROJECT_NAME} PRIVATE Qt5::X11Extras)
endif()

################################# CMark ########################################

if(RS_GUI_CMARK)
	find_package(cmark REQUIRED)
	target_compile_definitions(${PROJECT_NAME} PRIVATE USE_CMARK)
	target_link_libraries(${PROJECT_NAME} PRIVATE cmark::cmark)
endif(RS_GUI_CMARK)

################################# LibSam v3 ####################################

# sam3 is used in gui/settings/ServerPage.cpp
if(RS_USE_I2P_SAM3)
	find_package(sam3 REQUIRED)
	target_link_libraries(${PROJECT_NAME} PRIVATE sam3::libsam3)
	target_compile_definitions(${PROJECT_NAME}
		PRIVATE
		RS_USE_I2P_SAM3
		RS_USE_I2P_SAM3_LIBSAM3
	)
endif()

################################################################################
### platforms

if(UNIX)
	# Large File Support for 32-bit builds (64-bit off_t). No-op on 64-bit
	# platforms; kept because the GUI does perform some direct file I/O and this
	# keeps it consistent with libretroshare.
	target_compile_definitions(${PROJECT_NAME} PRIVATE _FILE_OFFSET_BITS=64)
	target_link_options(${PROJECT_NAME} PRIVATE -rdynamic)
endif()


if(WIN32)
	set_target_properties(${PROJECT_NAME} PROPERTIES
		ARCHIVE_OUTPUT_NAME "retroshare-gui"
	)

	if(NOT RS_DEVELOPMENT_BUILD AND NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
		# Hide console window on Windows for release builds (Release)
		set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)
	endif()

	if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
		# Exclude static libs from the re-exported symbol set. This matters for the
		# Ninja generator on Windows: there the GUI otherwise re-exports the symbols
		# of these static libs, and plugins that also link the same lib (e.g.
		# libsam3.a in FeedReader) then fail with "multiple definition" at link time.
		# The MSYS Makefiles generator does not hit this, but excluding them keeps
		# Ninja builds working too.
		target_link_options(${PROJECT_NAME}
			PRIVATE
			"-Wl,--export-all-symbols"
			"-Wl,--exclude-libs,libbitdht.a:librnp.a:libsexpp.a:libsam3.a"
		)
	endif()
endif()


if(APPLE)
	set_target_properties(${PROJECT_NAME} PROPERTIES MACOSX_BUNDLE TRUE)
endif()

################################################################################
### subdirectories

add_subdirectory(src)
add_subdirectory(data)

################################################################################
### install

set(BUILD_CONFIG_DIR "${PROJECT_BINARY_DIR}/cmake")
set(INSTALL_CONFIG_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}")
set(TARGET_NAMESPACE retroshare)
set(INSTALL_COMPONENT COMPONENT retroshare-gui)
if(CMAKE_EXPERIMENTAL_EXPORT_PACKAGE_DEPENDENCIES)
	set(EXPORT_PACKAGE_DEPENDENCIES EXPORT_PACKAGE_DEPENDENCIES)
endif()

file(GLOB INSTALL_MODULES LIST_DIRECTORIES FALSE
	"${PROJECT_SOURCE_DIR}/mk/cmake/*"
	"${PROJECT_SOURCE_DIR}/mk/cmake/fetch_provider_packages/*"
)

if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.23)
	set(FILESET_HEADERS_INSTALL
		FILE_SET HEADERS DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/retroshare-gui
	)
else()
	set(FILESET_HEADERS_INSTALL
		INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/retroshare-gui
	)
	get_target_property(${PROJECT_NAME}_SOURCES ${PROJECT_NAME} SOURCES)
	foreach(source ${${PROJECT_NAME}_SOURCES})
		string(TOLOWER ${source} sourceLower)
		if(NOT sourceLower MATCHES "\\.(h|hh|h\\+\\+|hpp)\$")
			continue()
		endif()
		foreach(base_ ${PROJECT_SOURCE_DIR}/src ${PROJECT_BINARY_DIR}/src)
			string(FIND ${source} ${base_}/ basePos)
			if(basePos EQUAL 0)
				set(base ${base_})
				break()
			endif()
		endforeach()
		if(NOT basePos EQUAL 0)
			message(SEND_ERROR "couldn't find base for header ${source}")
			continue()
		endif()

		get_filename_component(dest ${source} DIRECTORY)
		file(RELATIVE_PATH dest ${base} ${dest})
		install(FILES ${source}
			DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/retroshare-gui/${dest}
			${INSTALL_COMPONENT}
		)
	endforeach()
endif()

install(TARGETS ${PROJECT_NAME}
	EXPORT ${PROJECT_NAME}_export
	${INSTALL_COMPONENT}
	${FILESET_HEADERS_INSTALL}
	BUNDLE
		DESTINATION ${CMAKE_INSTALL_BINDIR}
)


# If dependencies are fetched via FetchContent, then their targets might not
# have export sets, and this prevents us from using our own export set.
if(NOT FETCH_DEPENDENCY_PROVIDER)

	## config for install tree
	install(EXPORT ${PROJECT_NAME}_export
		DESTINATION "${INSTALL_CONFIG_DIR}"
		FILE ${PROJECT_NAME_LOWER}-targets.cmake
		NAMESPACE ${TARGET_NAMESPACE}::
		${INSTALL_COMPONENT}
		${EXPORT_PACKAGE_DEPENDENCIES}
	)
	include(CMakePackageConfigHelpers)
	configure_package_config_file(
		mk/cmake/retroshare-gui-config.cmake.in
		"${BUILD_CONFIG_DIR}/${PROJECT_NAME_LOWER}-config-install.cmake"
		INSTALL_DESTINATION "${INSTALL_CONFIG_DIR}"
	)
	write_basic_package_version_file(
		"${BUILD_CONFIG_DIR}/${PROJECT_NAME_LOWER}-config-version-install.cmake"
		COMPATIBILITY SameMinorVersion
	)
	foreach(config config config-version)
		install(FILES
			"${BUILD_CONFIG_DIR}/${PROJECT_NAME_LOWER}-${config}-install.cmake"
			DESTINATION "${INSTALL_CONFIG_DIR}"
			RENAME ${PROJECT_NAME_LOWER}-${config}.cmake
			${INSTALL_COMPONENT}
		)
	endforeach()
	install(FILES ${INSTALL_MODULES} DESTINATION ${INSTALL_CONFIG_DIR}/Modules)


	## config for build tree
	export(EXPORT ${PROJECT_NAME}_export
	  FILE "${BUILD_CONFIG_DIR}/${PROJECT_NAME_LOWER}-targets.cmake"
	  NAMESPACE ${TARGET_NAMESPACE}::
	  ${EXPORT_PACKAGE_DEPENDENCIES}
	)
	include(CMakePackageConfigHelpers)
	configure_package_config_file(
		mk/cmake/retroshare-gui-config.cmake.in
	  "${BUILD_CONFIG_DIR}/${PROJECT_NAME_LOWER}-config.cmake"
	  INSTALL_DESTINATION "${BUILD_CONFIG_DIR}"
		INSTALL_PREFIX "${PROJECT_BINARY_DIR}"
	)
	write_basic_package_version_file(
		"${BUILD_CONFIG_DIR}/${PROJECT_NAME_LOWER}-config-version.cmake"
	  COMPATIBILITY SameMinorVersion
	)
	file(COPY ${INSTALL_MODULES} DESTINATION ${BUILD_CONFIG_DIR}/Modules)

endif()


## Alias target for super-builds (brought in via add_subdirectory)
add_executable(${TARGET_NAMESPACE}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})


################################################################################
### packaging

if(PROJECT_IS_TOP_LEVEL)

## generators
set(CPACK_GENERATOR TXZ)
if(WIN32)
	set(CPACK_GENERATOR ZIP)
endif()
set(CPACK_SOURCE_GENERATOR ${CPACK_GENERATOR})
if(WIN32)
	list(APPEND CPACK_GENERATOR NSIS)
endif()
set(CPACK_PACKAGE_CHECKSUM SHA256)

## package metadata
set(CPACK_PACKAGE_NAME ${PROJECT_NAME_PRETTY})
set(CPACK_PACKAGE_VENDOR "RetroShare")
set(CPACK_PACKAGE_ICON ${PROJECT_SOURCE_DIR}/data/logos/128x128.png)
# set(CPACK_RESOURCE_FILE_README "${PROJECT_SOURCE_DIR}/README.md")
# set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/COPYING.txt")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION_FULL})

## NSIS generator
# set(CPACK_NSIS_MUI_ICON ${PROJECT_SOURCE_DIR}/data/logos/${APPSTREAM_ID}.ico)
set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL YES)
set(CPACK_NSIS_MODIFY_PATH YES)
set(CPACK_NSIS_IGNORE_LICENSE_PAGE YES)
set(CPACK_NSIS_DISPLAY_NAME "RetroShare-GUI")

## misc packaging options
set(CPACK_VERBATIM_VARIABLES TRUE)
set(CPACK_COMPONENTS_ALL retroshare-gui)
# if(CMAKE_CXX_FLAGS_${BUILD_TYPE_UPPER} MATCHES "(^| )-g( |$)")
#   set(CPACK_STRIP_FILES FALSE)
# else()
#   set(CPACK_STRIP_FILES TRUE)
# endif()
set(CPACK_SOURCE_IGNORE_FILES
  "${PROJECT_BINARY_DIR}/"
  "${PROJECT_SOURCE_DIR}/\\.git/"
  "${PROJECT_SOURCE_DIR}/build/"
)
set(CPACK_THREADS 0)

find_package(Git QUIET)
configure_file(mk/cmake/RSCPackConfig.cmake.in RSCPackConfig.cmake @ONLY)
set(CPACK_PROJECT_CONFIG_FILE ${CMAKE_CURRENT_BINARY_DIR}/RSCPackConfig.cmake)
include(CPack)

endif()
