################################################################################
# retroshare-gui/CMakeLists.txt                                                #
# Copyright (C) 2022, Retroshare team <retroshare.team@gmailcom>               #
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

cmake_minimum_required (VERSION 3.18.0)
project(retroshare-gui VERSION 0.6.7 LANGUAGES CXX C)

include(CMakeDependentOption)

set(
	RS_BIN_INSTALL_DIR
	"${CMAKE_INSTALL_PREFIX}/bin"
	CACHE PATH
	"Path where to install retroshare-service compiled binary" )

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

################################ QT FILES #######################################

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
# AUTOUIC is intentionally OFF: the .ui forms are collected in src/CMakeLists.txt
# (RS_GUI_FORMS) and processed explicitly below with qt_wrap_ui/qt5_wrap_ui, which
# generate the ui_*.h headers into the build directory (added to the include path).
# Enabling AUTOUIC as well would process the same forms a second time.
set(CMAKE_AUTOUIC OFF)

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

list( APPEND RS_LINK_LIBRARIES
	Qt${QT_VERSION_MAJOR}::Multimedia Qt${QT_VERSION_MAJOR}::Widgets
	Qt${QT_VERSION_MAJOR}::Xml Qt${QT_VERSION_MAJOR}::Network
	Qt${QT_VERSION_MAJOR}::PrintSupport)
if(QT_VERSION_MAJOR EQUAL 6)
	list( APPEND RS_LINK_LIBRARIES Qt6::Core5Compat )
endif()
if(APPLE)
	list( APPEND RS_LINK_LIBRARIES Qt${QT_VERSION_MAJOR}::Svg )
endif()
if(UNIX AND NOT APPLE AND QT_VERSION_MAJOR EQUAL 5)
	list( APPEND RS_LINK_LIBRARIES Qt5::X11Extras )
endif()

################################################################################

set(FETCHCONTENT_QUIET OFF)
include(FetchContent)

find_package(Git REQUIRED)

################################################################################

if(RS_DEVELOPMENT_BUILD)
	set(CMAKE_VERBOSE_MAKEFILE ON)
	set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif(RS_DEVELOPMENT_BUILD)

################################################################################

include(src/CMakeLists.txt)

# Versionless command, available for both Qt5 (>=5.15) and Qt6. Called once to
# avoid generating duplicate ui_*.h rules.
qt_wrap_ui(RS_UI_HEADERS ${RS_GUI_FORMS})

# Compatibility shim: src/gui/AboutWidget.cpp unconditionally does
# #include "include/qmake_info.h" and prints Rs_qmake_DEFINES / Rs_qmake_CONFIG in
# the About dialog. That header is normally produced by the qmake build (and is
# git-ignored), so a CMake build has nothing to include. We generate a minimal
# stand-in here to keep AboutWidget compiling.
# TODO: replace this by generating the file from the real CMake configuration via
# configure_file(), or make AboutWidget obtain the build info without a
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

if(WIN32)
    list(APPEND RS_GUI_SOURCES src/gui/images/retroshare_win.rc)
elseif(APPLE)
    list(APPEND RS_GUI_SOURCES src/gui/common/MacDockIconHandler.mm src/rsMacIcon.icns)
    set_source_files_properties(src/gui/common/MacDockIconHandler.mm PROPERTIES COMPILE_FLAGS "-x objective-c++")
    set_source_files_properties(src/rsMacIcon.icns PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
endif()

set(GUI_TYPE "")
if(WIN32)
	# Hide the console window on Windows for production builds (Release), 
	# but keep it visible for Debug and Development builds to view stdout/stderr logs.
	if(NOT RS_DEVELOPMENT_BUILD AND NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
		set(GUI_TYPE WIN32)
	endif()
elseif(APPLE)
	set(GUI_TYPE MACOSX_BUNDLE)
endif()

add_executable(${PROJECT_NAME} ${GUI_TYPE} ${RS_GUI_SOURCES} ${RS_IMPLEMENTATION_HEADERS} ${RS_UI_HEADERS} ${RS_GUI_QTRESOURCES})
# The GUI executable is named "retroshare-gui" on all platforms (it was
# historically just "retroshare"; renamed for clarity vs retroshare-service).
set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_NAME "retroshare-gui")
set_target_properties(${PROJECT_NAME} PROPERTIES ENABLE_EXPORTS TRUE)

if(APPLE)
	set_target_properties(${PROJECT_NAME} PROPERTIES
		MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/src/Info.plist"
	)
endif()

if(WIN32)
	set_target_properties(${PROJECT_NAME} PROPERTIES
		ARCHIVE_OUTPUT_NAME "retroshare-gui"
	)
	if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
		# Exclude static libs from the re-exported symbol set. This matters for the
		# Ninja generator on Windows: there the GUI otherwise re-exports the symbols
		# of these static libs, and plugins that also link the same lib (e.g.
		# libsam3.a in FeedReader) then fail with "multiple definition" at link time.
		# The MSYS Makefiles generator does not hit this, but excluding them keeps
		# Ninja builds working too.
		target_link_options(${PROJECT_NAME} PRIVATE "-Wl,--export-all-symbols" "-Wl,--exclude-libs,libretroshare.a:libbitdht.a:librnp.a:libsexpp.a:libsam3.a")
	endif()
endif()
install(TARGETS ${PROJECT_NAME} DESTINATION ${RS_BIN_INSTALL_DIR})


if(RS_USE_NATIVE_DIALOGS)
	target_compile_definitions(${PROJECT_NAME} PRIVATE RS_NATIVEDIALOGS)
endif()

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/)

################################################################################

if(RS_DEVELOPMENT_BUILD)
	target_compile_options(${PROJECT_NAME} PRIVATE "-O0")
endif(RS_DEVELOPMENT_BUILD)

################################################################################

if(NOT TARGET retroshare)
	get_filename_component(LIBRETROSHARE_DEVEL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../libretroshare/" ABSOLUTE)
	if(EXISTS "${LIBRETROSHARE_DEVEL_DIR}/CMakeLists.txt" )
		message(
			STATUS
			"libretroshare submodule found at ${LIBRETROSHARE_DEVEL_DIR} using it" )
		add_subdirectory(
			"${LIBRETROSHARE_DEVEL_DIR}" "${CMAKE_BINARY_DIR}/libretroshare" )
	else()
		FetchContent_Declare(
			libretroshare
			GIT_REPOSITORY "https://gitlab.com/RetroShare/libretroshare.git"
			GIT_TAG "origin/master"
			GIT_SHALLOW TRUE
			GIT_PROGRESS TRUE
			TIMEOUT 10
		)
		FetchContent_MakeAvailable(libretroshare)
		set(LIBRETROSHARE_DEVEL_DIR "${libretroshare_SOURCE_DIR}")
	endif()
else()
	get_target_property(LIBRETROSHARE_DEVEL_DIR retroshare SOURCE_DIR)
endif()

target_include_directories(${PROJECT_NAME} PRIVATE ${LIBRETROSHARE_DEVEL_DIR}/src/)

################################################################################

if(RS_SERVICE_DESKTOP)
	if(UNIX AND NOT APPLE)
		install(
			FILES data/retroshare.svg
			DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/scalable/ )

		install(
			FILES data/retroshare.xpm
			DESTINATION ${CMAKE_INSTALL_PREFIX}/share/pixmaps/ )

		install(
			FILES data/24x24/apps/retroshare.png
			DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/24x24/apps/retroshare.png )

		install(
			FILES data/48x48/apps/retroshare.png
			DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/48x48/apps/retroshare.png )

		install(
			FILES data/64x64/apps/retroshare.png
			DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/64x64/apps/retroshare.png )

		install(
			FILES data/128x128/apps/retroshare.png
			DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/128x128/apps/retroshare.png )

		install(
			FILES data/retroshare.desktop
			DESTINATION ${CMAKE_INSTALL_PREFIX}/data/retroshare.desktop )

		install(
			FILES gui/qss/chat/Bubble gui/qss/chat/Bubble_Compact
			DESTINATION ${CMAKE_INSTALL_PREFIX}/data/stylesheets/ )

		install(
			FILES src/sounds/ src/qss/
			DESTINATION ${CMAKE_INSTALL_PREFIX}/ )
	endif(UNIX AND NOT APPLE)
endif(RS_SERVICE_DESKTOP)

################################################################################

if(RS_JSON_API)
	target_compile_definitions(${PROJECT_NAME} PUBLIC RS_JSONAPI)
endif(RS_JSON_API)

if(RS_WEBUI)
	target_compile_definitions(${PROJECT_NAME} PRIVATE RS_WEBUI)
endif(RS_WEBUI)

################################# CMark ########################################

if(RS_GUI_CMARK)
	set(CMARK_DEVEL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../supportlibs/cmark/")
	if(EXISTS "${CMARK_DEVEL_DIR}/CMakeLists.txt" )
		message( STATUS "cmark submodule found at ${CMARK_DEVEL_DIR} using it" )
		add_subdirectory( "${CMARK_DEVEL_DIR}" "${CMAKE_BINARY_DIR}/cmark" )
		set(CMARK_INCLUDE_DIR "${CMARK_DEVEL_DIR}/src")
	else()
		FetchContent_Declare(
			cmark
			GIT_REPOSITORY "https://github.com/commonmark/cmark.git"
			GIT_TAG "origin/master"
			GIT_SHALLOW TRUE
			GIT_PROGRESS TRUE
			TIMEOUT 10
			)
		FetchContent_MakeAvailable(cmark)
		set(CMARK_INCLUDE_DIR "${cmark_SOURCE_DIR}/src")
	endif()

	target_compile_definitions(${PROJECT_NAME} PRIVATE USE_CMARK)
	target_link_libraries(${PROJECT_NAME} PRIVATE libcmark_static)
	target_include_directories(${PROJECT_NAME} PRIVATE "${CMARK_INCLUDE_DIR}")
endif(RS_GUI_CMARK)

################################# LibSam v3 ####################################

# sam3 is used directly by the GUI (gui/settings/ServerPage.cpp includes
# <libsam3.h> and calls sam3CloseConnection()), so it is linked here too, not
# only inside libretroshare.
if(RS_USE_I2P_SAM3)
	target_link_libraries(${PROJECT_NAME} PRIVATE sam3)
endif()

################################################################################
#                                    TODO                                      #
################################################################################

#   # Auto detect installed version of cmark
#   rs_gui_cmark {
#   	DEFINES *= USE_CMARK
#   	no_rs_cross_compiling {
#   		message("Using compiled cmark")
#   		CMARK_SRC_PATH=$$clean_path($${RS_SRC_PATH}/supportlibs/cmark)
#   		CMARK_BUILD_PATH=$$clean_path($${RS_BUILD_PATH}/supportlibs/cmark/build)
#   		INCLUDEPATH *= $$clean_path($${CMARK_SRC_PATH}/src/)
#   		DEPENDPATH *= $$clean_path($${CMARK_SRC_PATH}/src/)
#   		QMAKE_LIBDIR *= $$clean_path($${CMARK_BUILD_PATH}/)
#   		# Using sLibs would fail as libcmark.a is generated at compile-time
#   		LIBS *= -L$$clean_path($${CMARK_BUILD_PATH}/src/) -lcmark
#   
#   		DUMMYCMARKINPUT = FORCE
#   		CMAKE_GENERATOR_OVERRIDE=""
#   		win32-g++|win32-clang-g++:CMAKE_GENERATOR_OVERRIDE="-G \"MSYS Makefiles\""
#   		gencmarklib.name = Generating libcmark.
#   		gencmarklib.input = DUMMYCMARKINPUT
#   		gencmarklib.output = $$clean_path($${CMARK_BUILD_PATH}/src/libcmark.a)
#   		gencmarklib.CONFIG += target_predeps combine
#   		gencmarklib.variable_out = PRE_TARGETDEPS
#   		gencmarklib.commands = \
#   		    cd $${RS_SRC_PATH} && ( \
#   		    git submodule update --init supportlibs/cmark ; \
#   		    cd $${CMARK_SRC_PATH} ; \
#   		    true ) && \
#   		    mkdir -p $${CMARK_BUILD_PATH} && cd $${CMARK_BUILD_PATH} && \
#   		    cmake \
#   		        -DCMAKE_CXX_COMPILER=$$QMAKE_CXX \
#                           \"-DCMAKE_CXX_FLAGS=$${QMAKE_CXXFLAGS}\" \
#   		        $${CMAKE_GENERATOR_OVERRIDE} \
#   		        -DCMAKE_INSTALL_PREFIX=. \
#   		        -B. \
#   		        -H$$shell_path($${CMARK_SRC_PATH}) && \
#   		    $(MAKE)
#   		QMAKE_EXTRA_COMPILERS += gencmarklib
#   	} else {
#   		message("Using systems cmark")
#   		sLibs *= libcmark
#   	}
#   }

################################# Linux ##########################################
# Put lib dir in QMAKE_LFLAGS so it appears before -L/usr/lib

if(UNIX)
	find_package(PkgConfig REQUIRED)

	if(NOT APPLE)
		# X11 + XScreenSaver are used directly by the GUI for idle detection
		# (src/idle/idle_platform.cpp, guarded by HAVE_XSS). NOTE: the XScreenSaver
		# extension only works under X11/XWayland, not native Wayland; modernising
		# idle detection for Wayland is a separate effort.
		pkg_check_modules(REQUIRED x11)
		pkg_check_modules(X11 REQUIRED x11)
		pkg_check_modules(XSCRNSAVER REQUIRED xscrnsaver)

		list( APPEND RS_LINK_LIBRARIES ${X11_LIBRARIES} )
		list( APPEND RS_LINK_LIBRARIES ${XSCRNSAVER_LIBRARIES} )

		target_include_directories(${PROJECT_NAME} PRIVATE ${X11_INCLUDE_DIRS})
		target_compile_options(${PROJECT_NAME} PRIVATE ${X11_CFLAGS_OTHER})

		target_include_directories(retroshare-gui PRIVATE ${XSCRNSAVER_INCLUDE_DIRS})
		target_compile_options(retroshare-gui PRIVATE ${XSCRNSAVER_CFLAGS_OTHER})

		target_compile_definitions(${PROJECT_NAME} PRIVATE HAVE_XSS)
	endif()

	# Large File Support for 32-bit builds (64-bit off_t). No-op on 64-bit
	# platforms; kept because the GUI does perform some direct file I/O and this
	# keeps it consistent with libretroshare.
	target_compile_definitions(${PROJECT_NAME} PRIVATE _FILE_OFFSET_BITS=64)
	target_link_options(${PROJECT_NAME} PRIVATE -rdynamic)
endif(UNIX)

if(RS_SANITIZE)
	list( APPEND RS_LINK_LIBRARIES asan )
	list( APPEND RS_LINK_LIBRARIES ubsan )
endif(RS_SANITIZE)

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
#                       	PRE_TARGETDEPS *= ../../openpgpsdk/src/lib/libops.a
#                       
#                       	LIBS *= ../../libretroshare/src/lib/libretroshare.a
#                       	LIBS *= ../../openpgpsdk/src/lib/libops.a -lbz2 -lbsd
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

################################ GENERAL #######################################

target_link_libraries(${PROJECT_NAME} PRIVATE ${RS_LINK_LIBRARIES})


# Controlled by an option, like in libretroshare/CMakeLists.txt: silence the
# deprecation warnings unless the developer explicitly asks to see them.
if(NOT RS_WARN_DEPRECATED)
	target_compile_options(${PROJECT_NAME} PRIVATE -Wno-deprecated -Wno-deprecated-declarations)
	target_compile_definitions(${PROJECT_NAME} PUBLIC RS_NO_WARN_DEPRECATED)
endif(NOT RS_WARN_DEPRECATED)
target_compile_definitions(${PROJECT_NAME} PRIVATE RS_RELEASE_VERSION )
# NOTE: TARGET is NOT the executable name: it is the single-instance IPC socket
# name used in main.cpp / rshare.cpp (connectToServer(TARGET)). Kept as
# "retroshare" so the single-instance lock/socket name stays unchanged.
target_compile_definitions(${PROJECT_NAME} PRIVATE TARGET=\"retroshare\")
target_compile_definitions(${PROJECT_NAME} PRIVATE RS_DIRECT_CHAT)

# Match the actual option name RS_GXSCIRCLES (defined above). It was previously
# tested as RS_GXS_CIRCLES (extra underscore), so RS_USE_CIRCLES was never defined.
if(RS_GXSCIRCLES)
	target_compile_definitions(${PROJECT_NAME} PRIVATE RS_USE_CIRCLES )
endif(RS_GXSCIRCLES)

# NOTE: the GUI does NOT use botan, json-c, z or bz2 directly (it uses rapidjson
# for JSON). They are transitive dependencies of librnp (PGP), pulled in here only
# because the statically-linked librnp does not propagate them. Ideally librnp
# (or libretroshare) should export these as link-interface deps so consumers don't
# have to repeat them; that belongs to libretroshare and is left as a follow-up.
find_library(BOTAN_LIBRARY NAMES botan botan-3 botan-2 libbotan-3 libbotan-2 REQUIRED)

if(WIN32)
    # Robust resolution including RNP and sexpp inside the link group for Windows MinGW
    target_link_libraries(${PROJECT_NAME} PRIVATE -Wl,--start-group retroshare librnp sexpp z bz2 json-c ${BOTAN_LIBRARY} -Wl,--end-group)
elseif(APPLE)
    # macOS specific behavior: skip Linux-specific linker flags
    target_link_libraries(${PROJECT_NAME} PRIVATE retroshare z bz2 json-c ${BOTAN_LIBRARY})
else()
    # Default behavior for Linux
    target_link_libraries(${PROJECT_NAME} PRIVATE retroshare)
    target_link_libraries(${PROJECT_NAME} PRIVATE -Wl,--no-as-needed z bz2 json-c ${BOTAN_LIBRARY} -Wl,--as-needed)
endif()

if(RS_PLUGINS AND CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
	set(PLUGINS_DEVEL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../plugins/")
	if(EXISTS "${PLUGINS_DEVEL_DIR}/CMakeLists.txt")
		message(STATUS "plugins source found at ${PLUGINS_DEVEL_DIR} using it")
		add_subdirectory("${PLUGINS_DEVEL_DIR}" "${CMAKE_BINARY_DIR}/plugins")
	endif()
endif()
