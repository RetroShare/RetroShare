# ------------------------------------------------------------------------ *\
# src/gui/CMakeLists.txt
# This file is part of RetroShare-GUI.
#
# Copyright (C) 2018      RetroShare Team <retroshare.team@gmail.com>
# Copyright (C) 2026      David Bears <dbear4q@gmail.com>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as
# published by the Free Software Foundation, either version 3 of the
# License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
# ------------------------------------------------------------------------ */

target_sources(${PROJECT_NAME}
	PUBLIC FILE_SET HEADERS
	BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}

	PRIVATE FILE_SET private_HEADERS TYPE HEADERS
	BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
)
if(CMAKE_VERSION VERSION_GREATER_EQUAL "4.4")
	target_sources(${PROJECT_NAME}
		PRIVATE FILE_SET SOURCES
		PRIVATE FILE_SET ui TYPE SOURCES
		PRIVATE FILE_SET resources TYPE SOURCES
	)
	set("PRIVATE_FILESET_SOURCES_FILES" PRIVATE FILE_SET SOURCES FILES)
	set("PRIVATE_FILESET_UI_FILES" PRIVATE FILE_SET ui FILES)
	set("PRIVATE_FILESET_RESOURCES_FILES" PRIVATE FILE_SET resources FILES)
else()
	set("PRIVATE_FILESET_SOURCES_FILES" PRIVATE)
	set("PRIVATE_FILESET_UI_FILES" PRIVATE)
	set("PRIVATE_FILESET_RESOURCES_FILES" PRIVATE)
endif()

add_subdirectory(control)
add_subdirectory(gui)
if(RS_IDLE)
	add_subdirectory(idle)
endif()
add_subdirectory(lang)
add_subdirectory(retroshare-gui)
add_subdirectory(TorControl)
add_subdirectory(util)

target_sources(${PROJECT_NAME}
	PUBLIC FILE_SET HEADERS FILES

	PRIVATE FILE_SET private_HEADERS FILES
	rshare.h

	${PRIVATE_FILESET_SOURCES_FILES}
	rsguiversion.h
	rshare.cpp
	main.cpp

	${PRIVATE_FILESET_UI_FILES}

	${PRIVATE_FILESET_RESOURCES_FILES}
)


# Compatibility shim: gui/AboutWidget.cpp unconditionally does
# `#include "include/qmake_info.h"`` and prints
# Rs_qmake_DEFINES / Rs_qmake_CONFIG in the About dialog. That header is
# normally produced by the qmake build (and is git-ignored), so a CMake build
# has nothing to include. We generate a minimal stand-in here to keep
# AboutWidget compiling.
# TODO: replace this by generating the file from the real CMake configuration
# via configure_file(), or make AboutWidget obtain the build info without a
# qmake-specific header.
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/include")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/include/qmake_info.h"
	"#ifndef INFO_H\n"
	"#define INFO_H\n\n"
	"#include <QString>\n\n"
	"static const QString Rs_qmake_DEFINES = QStringLiteral(\"CMake Build\");\n"
	"static const QString Rs_qmake_CONFIG = QStringLiteral(\"CMake Build\");\n\n"
	"#endif // INFO_H\n"
)
target_sources(${PROJECT_NAME}
	PRIVATE FILE_SET private_HEADERS FILES
	"${CMAKE_CURRENT_BINARY_DIR}/include/qmake_info.h"
)

if(APPLE)
	target_sources(${PROJECT_NAME}
		${PRIVATE_FILESET_RESOURCES_FILES}
		rsMacIcon.icns
	)
	set_source_files_properties(rsMacIcon.icns PROPERTIES
		MACOSX_PACKAGE_LOCATION "Resources"
	)
	set_target_properties(${PROJECT_NAME} PROPERTIES
		MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist"
	)
endif()
