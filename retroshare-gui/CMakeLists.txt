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
project(retroshare-gui VERSION 0.6.6 LANGUAGES CXX)

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

option(
	RS_JSON_API
	"Use restbed to expose libretroshare as JSON API via HTTP"
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

cmake_dependent_option(
	RS_SERVICE_TERMINAL_WEBUI_PASSWORD
	"Enable settin Web UI password via terminal in retroshare-service"
	OFF
	"RS_WEBUI"
	ON )

cmake_dependent_option(
	RS_WEBUI
	"Install RetroShare Web UI"
	OFF
	"RS_JSON_API"
	ON )

################################ QT FILES #######################################

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package( Qt5 COMPONENTS Core         REQUIRED)
find_package( Qt5 COMPONENTS Widgets      REQUIRED)
find_package( Qt5 COMPONENTS Xml          REQUIRED)
find_package( Qt5 COMPONENTS Network      REQUIRED)
find_package( Qt5 COMPONENTS Multimedia   REQUIRED)
find_package( Qt5 COMPONENTS PrintSupport REQUIRED)

list( APPEND RS_LINK_LIBRARIES Qt5::Multimedia Qt5::Widgets Qt5::Xml Qt5::Network Qt5::PrintSupport)

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

qt5_wrap_ui(RS_UI_HEADERS ${RS_GUI_FORMS})

add_executable(${PROJECT_NAME} ${RS_GUI_SOURCES} ${RS_UI_HEADERS} ${RS_GUI_QTRESOURCES})
install(TARGETS ${PROJECT_NAME} DESTINATION ${RS_BIN_INSTALL_DIR})

include_directories( ${CMAKE_BINARY_DIR} )

################################################################################

if(RS_DEVELOPMENT_BUILD)
	target_compile_options(${PROJECT_NAME} PRIVATE "-O0")
endif(RS_DEVELOPMENT_BUILD)

################################################################################

set(LIBRETROSHARE_DEVEL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../libretroshare/")
if(EXISTS "${LIBRETROSHARE_DEVEL_DIR}/.git" )
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

################################# CMark ########################################

if(RS_GUI_CMARK)
	set(CMARK_DEVEL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../supportlibs/cmark/")
	if(EXISTS "${LIBRETROSHARE_DEVEL_DIR}/.git" )
		message( STATUS "cmark submodule found at ${LIBRETROSHARE_DEVEL_DIR} using it" )
		add_subdirectory( "${LIBRETROSHARE_DEVEL_DIR}" "${CMAKE_BINARY_DIR}/cmark" )
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
	endif()
endif(RS_GUI_CMARK)

################################# LibSam v3 ####################################

set(SAM3_DEVEL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../supportlibs/libsam3/")
if(EXISTS "${SAM3_DEVEL_DIR}/.git" )
	message( STATUS "libsam3 submodule found at ${SAM3_DEVEL_DIR} using it" )
	add_subdirectory( "${SAM3_DEVEL_DIR}" "${CMAKE_BINARY_DIR}/libsam3" )
else()
	FetchContent_Declare(
		libsam3
		GIT_REPOSITORY "https://github.com/i2p/libsam3.git"
		GIT_TAG "origin/master"
		GIT_SHALLOW TRUE
		GIT_PROGRESS TRUE
		TIMEOUT 10
		)
	FetchContent_MakeAvailable(libsam3)
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

	pkg_check_modules(X11 REQUIRED x11)
	pkg_check_modules(XSCRNSAVER REQUIRED xscrnsaver)

	list( APPEND RS_LINK_LIBRARIES ${X11_LIBRARIES} )
	list( APPEND RS_LINK_LIBRARIES ${XSCRNSAVER_LIBRARIES} )

	target_include_directories(${PROJECT_NAME} PRIVATE ${X11_INCLUDE_DIRS})
	target_compile_options(${PROJECT_NAME} PRIVATE ${X11_CFLAGS_OTHER})

	target_include_directories(retroshare-gui PRIVATE ${XSCRNSAVER_INCLUDE_DIRS})
	target_compile_options(retroshare-gui PRIVATE ${XSCRNSAVER_CFLAGS_OTHER})

	target_compile_definitions(${PROJECT_NAME} PRIVATE HAVE_XSS)
	target_compile_definitions(${PROJECT_NAME} PRIVATE _FILE_OFFSET_BITS=64)
	target_link_options(${PROJECT_NAME} PRIVATE LINKER:-rdynamic)
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

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/)

set( CMAKE_CXX_FLAGS "-Wno-deprecated-declarations" )
target_compile_definitions(${PROJECT_NAME} PUBLIC  RS_NO_WARN_DEPRECATED )
target_compile_definitions(${PROJECT_NAME} PRIVATE RS_RELEASE_VERSION )
target_compile_definitions(${PROJECT_NAME} PRIVATE TARGET=\"retroshare\")

if(RS_GXS_CIRCLES)
	target_compile_definitions(${PROJECT_NAME} PRIVATE RS_USE_CIRCLES )
endif(RS_GXS_CIRCLES)

#add_dependencies(${PROJECT_NAME} libretroshare)

