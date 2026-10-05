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

cmake_minimum_required(VERSION 3.24...4.4)

project(retroshare-gui VERSION 0.6.7 LANGUAGES CXX C)

include(GNUInstallDirs)
include(CMakeDependentOption)

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
	 ON )

option(RS_JSON_API "Use restbed to expose libretroshare as JSON API via HTTP" ON)

# Mirrors the option of the same name in libretroshare/CMakeLists.txt.
option(
	RS_WARN_DEPRECATED
	"Print warnings about deprecated RetroShare components usage during build"
	OFF )

option(
	RS_USE_NATIVE_DIALOGS
	"Use the operating system native file/font/directory dialogs. When OFF, Qt's \
	 own (non-native) dialogs are used instead (QFileDialog::DontUseNativeDialog)."
	OFF )

option(
	RS_SERVICE_DESKTOP
	"Install icons and shortcuts for desktop environements"
	OFF )

option(
	RS_SERVICE_TERMINAL_LOGIN
	"Enable RetroShare login via terminal"
	ON )

option( RS_GXSGUI      "Enable GXS services in GUI" ON )
option( RS_GXSCHANNELS "Enable GXS channels in GUI" ON )
option( RS_GXSFORUMS   "Enable GXS forums in GUI"   ON )
option( RS_GXSPOSTED   "Enable GXS posted in GUI"   ON )
option( RS_GXSCIRCLES  "Enable GXS circles in GUI"  ON )
option( RS_GXSTHEWIRE  "Enable GXS The Wire in GUI" OFF)
option( RS_GXSWIKIPOS  "Enable GXS Wiki in GUI"     OFF)
option( RS_GUI_CMARK    "Enable CommonMark support in GUI"  OFF )
set(RS_GXSIDENTITIES ON CACHE BOOL "Enable GXS identities in GUI" FORCE)
set(RS_IDLE ON CACHE BOOL "Enable Idle support" FORCE)

cmake_dependent_option(
	RS_SERVICE_TERMINAL_WEBUI_PASSWORD
	"Enable settin Web UI password via terminal in retroshare-service"
	OFF
	"RS_WEBUI"
	OFF )

cmake_dependent_option(
	RS_WEBUI
	"Install RetroShare Web UI"
	OFF
	"RS_JSON_API"
	OFF )

option(
	RS_PLUGINS
	"Build RetroShare Plugins"
	OFF )

################################################################################
### dependencies

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/mk/cmake")
set(CMAKE_FIND_PACKAGE_PREFER_CONFIG TRUE)

find_package(RetroShare REQUIRED)

################################################################################
### version

set(RSGUI_VERSION_SUFFIX "" CACHE STRING
	"Custom version suffix to use; mostly useful for downstream packaging."
)
include(GetGitRevisionDescription)
if(DEFINED RSGUI_VERSION_SUFFIX AND NOT (RSGUI_VERSION_SUFFIX STREQUAL ""))
	set(FULL_PROJECT_VERSION "${PROJECT_VERSION}-${RSGUI_VERSION_SUFFIX}")
else()
	git_describe_working_tree(FULL_PROJECT_VERSION
		--tags --long --always --dirty --match v*.*.*
	)
	if(FULL_PROJECT_VERSION)
		string(SUBSTRING "${FULL_PROJECT_VERSION}" 1 -1 FULL_PROJECT_VERSION)
	else()
		set(FULL_PROJECT_VERSION "${PROJECT_VERSION}-unknown")
	endif()
endif()
message(STATUS "RetroShare-GUI version: ${FULL_PROJECT_VERSION}")

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
	"RS_GUI_VERSION=\"${FULL_PROJECT_VERSION}\""
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
find_package( Qt6 QUIET COMPONENTS Core )
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

################################################################################
### install

set(INSTALL_COMPONENT COMPONENT retroshare-gui)

install(TARGETS ${PROJECT_NAME}
	EXPORT ${PROJECT_NAME}_export
	${INSTALL_COMPONENT}
	FILE_SET HEADERS
		DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/retroshare-gui"
	BUNDLE
		DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(UNIX AND NOT APPLE AND RS_SERVICE_DESKTOP)
	install(
		FILES data/retroshare.svg
		DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/"
		${INSTALL_COMPONENT}
	)

	foreach(size 24x24 48x48 64x64 128x128)
		install(
			FILES data/${size}/apps/retroshare.png
			DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/${size}/apps/"
			${INSTALL_COMPONENT}
		)
	endforeach()

	install(
		FILES data/retroshare.xpm
		DESTINATION "${CMAKE_INSTALL_DATADIR}/pixmaps/"
		${INSTALL_COMPONENT}
	)

	install(
		FILES data/retroshare-gui.desktop
		DESTINATION "${CMAKE_INSTALL_DATADIR}/applications/"
		${INSTALL_COMPONENT}
	)

	install(
		DIRECTORY src/gui/qss/chat/Bubble src/gui/qss/chat/Bubble_Compact
		DESTINATION "${RS_INSTALL_DATADIR}/stylesheets/"
		${INSTALL_COMPONENT}
	)

	install(
		DIRECTORY src/sounds/ src/qss/ # TODO: move from src to data
		DESTINATION "${RS_INSTALL_DATADIR}"
		${INSTALL_COMPONENT}
	)
endif()

## TODO: config packages, alias target



#                       #################### Cross compilation for windows under Linux ###################
#
#                       win32-x-g++ {
#                       		OBJECTS_DIR = temp/win32-x-g++/obj
#
#                       		LIBS += ../../../../lib/win32-x-g++-v0.5/libssl.a
#                       		LIBS += ../../../../lib/win32-x-g++-v0.5/libcrypto.a
#                       		LIBS += ../../../../lib/win32-x-g++-v0.5/libgpgme.dll.a
#                       		LIBS += ../../../../lib/win32-x-g++-v0.5/libminiupnpc.a
#                       		LIBS += ../../../../lib/win32-x-g++-v0.5/libz.a
#                       		LIBS += -L${HOME}/.wine/drive_c/pthreads/lib -lpthreadGCE2
#                       		LIBS += -lQtUiTools
#                       		LIBS += -lws2_32 -luuid -lole32 -liphlpapi -lcrypt32 -gdi32
#                       		LIBS += -lole32 -lwinmm
#
#                       		DEFINES *= WINDOWS_SYS WIN32 WIN32_CROSS_UBUNTU
#
#                       		INCLUDEPATH += ../../../../gpgme-1.1.8/src/
#                       		INCLUDEPATH += ../../../../libgpg-error-1.7/src/
#
#                       		RC_FILE = gui/images/retroshare_win.rc
#                       }
#
#                       #################################### Windows #####################################
#
#                       win32-g++|win32-clang-g++ {
#                       	CONFIG(debug, debug|release) {
#                       		# show console output
#                       		CONFIG += console
#                       	} else {
#                       		CONFIG -= console
#                       	}
#
#                       	CONFIG(debug, debug|release) {
#                       	} else {
#                       		# Tell linker to use ASLR protection
#                       		QMAKE_LFLAGS += -Wl,-dynamicbase
#                       		# Tell linker to use DEP protection
#                       		QMAKE_LFLAGS += -Wl,-nxcompat
#                       	}
#
#                           # Fix linking error (ld.exe: Error: export ordinal too large) due to too
#                           # many exported symbols.
#                           !libretroshare_shared:QMAKE_LFLAGS+=-Wl,--exclude-libs,ALL
#
#                       	# Switch off optimization for release version
#                       	QMAKE_CXXFLAGS_RELEASE -= -O2
#                       	QMAKE_CXXFLAGS_RELEASE += -O0
#                       	QMAKE_CFLAGS_RELEASE -= -O2
#                       	QMAKE_CFLAGS_RELEASE += -O0
#
#                       	# Switch on optimization for debug version
#                       	#QMAKE_CXXFLAGS_DEBUG += -O2
#                       	#QMAKE_CFLAGS_DEBUG += -O2
#
#                       	OBJECTS_DIR = temp/obj
#
#                           dLib = ws2_32 gdi32 uuid ole32 iphlpapi crypt32 winmm
#                           LIBS *= $$linkDynamicLibs(dLib)
#
#                       	RC_FILE = gui/images/retroshare_win.rc
#
#                       	# export symbols for the plugins
#                       	LIBS += -Wl,--export-all-symbols,--out-implib,lib/libretroshare-gui.a
#
#                       	# create lib directory
#                       	isEmpty(QMAKE_SH) {
#                       		QMAKE_PRE_LINK = $(CHK_DIR_EXISTS) lib $(MKDIR) lib
#                       	} else {
#                       		QMAKE_PRE_LINK = $(CHK_DIR_EXISTS) lib || $(MKDIR) lib
#                       	}
#
#                       	greaterThan(QT_MAJOR_VERSION, 4) {
#                       		# Qt 5
#                       		RC_INCLUDEPATH += $$_PRO_FILE_PWD_/../../libretroshare/src
#                       	} else {
#                       		# Qt 4
#                       		QMAKE_RC += --include-dir=$$_PRO_FILE_PWD_/../../libretroshare/src
#                       	}
#                       }
#
#                       ##################################### MacOS ######################################
#
#                       macx {
#                       	# ENABLE THIS OPTION FOR Univeral Binary BUILD.
#                       	#CONFIG += ppc x86
#                       	#QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.4
#                       	QMAKE_INFO_PLIST = Info.plist
#                       	mac_icon.files = $$files($$PWD/rsMacIcon.icns)
#                       	mac_icon.path = Contents/Resources
#                       	QMAKE_BUNDLE_DATA += mac_icon
#                       	dplQSS.files = $$PWD/qss
#                       	dplQSS.path = Contents/Resources
#                       	QMAKE_BUNDLE_DATA += dplQSS
#                       	dplChatStyles.files = \
#                       		$$PWD/gui/qss/chat/Bubble \
#                       		$$PWD/gui/qss/chat/Bubble_Compact
#                       	dplChatStyles.path = Contents/Resources/stylesheets
#                       	QMAKE_BUNDLE_DATA += dplChatStyles
#                       #	mac_webui.files = $$files($$PWD/../../libresapi/src/webui)
#                       #	mac_webui.path = Contents/Resources
#                       #	QMAKE_BUNDLE_DATA += mac_webui
#
#                       	OBJECTS_DIR = temp/obj
#
#                       	CONFIG += version_detail_bash_script
#                       	LIBS += -lssl -lcrypto -lz
#                       	for(lib, LIB_DIR):exists($$lib/libminiupnpc.a){ LIBS += $$lib/libminiupnpc.a}
#                       	LIBS += -framework CoreFoundation
#                       	LIBS += -framework Security
#                       	LIBS += -framework Carbon
#
#                       	for(lib, LIB_DIR):LIBS += -L"$$lib"
#                       	for(bin, BIN_DIR):LIBS += -L"$$bin"
#
#                       	DEPENDPATH += . $$INC_DIR
#                       	INCLUDEPATH += . $$INC_DIR
#
#                       	#DEFINES *= MAC_IDLE # for idle feature
#                       	CONFIG -= uitools
#                       }
#
#                       ##################################### FreeBSD ######################################
#
#                       freebsd-* {
#                       	INCLUDEPATH *= /usr/local/include/gpgme
#                       	LIBS *= -lssl
#                       	LIBS *= -lgpgme
#                       	LIBS *= -lupnp
#                       	LIBS *= -lgnome-keyring
#
#                       	LIBS += -lsqlite3
#                       }
#
#                       ##################################### Haiku ######################################
#
#                       haiku-* {
#                       	PRE_TARGETDEPS *= ../../libretroshare/src/lib/libretroshare.a
#                       	PRE_TARGETDEPS *= ../../supportlibs/openpgpsdk/src/lib/libops.a
#
#                       	LIBS *= ../../libretroshare/src/lib/libretroshare.a
#                       	LIBS *= ../../supportlibs/openpgpsdk/src/lib/libops.a -lbz2 -lbsd
#                       	LIBS *= -lssl -lcrypto -lnetwork
#                       	LIBS *= -lgpgme
#                       	LIBS *= -lupnp
#                       	LIBS *= -lz
#                       	LIBS *= -lixml
#
#                       	LIBS += ../../supportlibs/pegmarkdown/lib/libpegmarkdown.a
#                       	LIBS += -lsqlite3
#                       }
#
#                       ##################################### OpenBSD ######################################
#
#                       openbsd-* {
#                       	INCLUDEPATH *= /usr/local/include
#
#                       	LIBS *= -lssl -lcrypto
#                       	LIBS *= -lgpgme
#                       	LIBS *= -lupnp
#                       	LIBS *= -lgnome-keyring
#                       	LIBS += -lsqlite3
#                       	LIBS *= -rdynamic
#                       }
#
#                       ################################### COMMON stuff ##################################
#
#                       wikipoos {
#                       	PRE_TARGETDEPS *= $$OUT_PWD/../../supportlibs/pegmarkdown/lib/libpegmarkdown.a
#                       	LIBS *= $$OUT_PWD/../../supportlibs/pegmarkdown/lib/libpegmarkdown.a
#                       	LIBS *= -lglib-2.0
#                       }
