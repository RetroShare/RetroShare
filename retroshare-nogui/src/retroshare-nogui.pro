!include("../../retroshare.pri"): error("Could not include file ../../retroshare.pri")

TEMPLATE = app
TARGET = RetroShare06-nogui
CONFIG += bitdht
#CONFIG += introserver
#CONFIG += sshserver
# webinterface, requires libmicrohttpd
CONFIG += webui
CONFIG -= qt xml gui
CONFIG += link_prl

################################# Linux ##########################################
linux-* {
	#CONFIG += version_detail_bash_script
	QMAKE_CXXFLAGS *= -D_FILE_OFFSET_BITS=64

	LIBS *= -rdynamic
}

unix {
	target.path = "$${BIN_DIR}"
	INSTALLS += target
}


#################### Cross compilation for windows under Linux ###################

win32-x-g++ {
	OBJECTS_DIR = temp/win32-x-g++/obj

	LIBS += ../../../../lib/win32-x-g++/libssl.a 
	LIBS += ../../../../lib/win32-x-g++/libcrypto.a 
	LIBS += ../../../../lib/win32-x-g++/libminiupnpc.a 
	LIBS += ../../../../lib/win32-x-g++/libz.a 
	LIBS += -L${HOME}/.wine/drive_c/pthreads/lib -lpthreadGCE2
	LIBS += -lws2_32 -luuid -lole32 -liphlpapi -lcrypt32 -gdi32
	LIBS += -lole32 -lwinmm

	RC_FILE = gui/images/retroshare_win.rc

	DEFINES *= WIN32
}

#################################### Windows #####################################

win32 {
	CONFIG += console
	OBJECTS_DIR = temp/obj
	RCC_DIR = temp/qrc
	UI_DIR  = temp/ui
	MOC_DIR = temp/moc

	# solve linker warnings because of the order of the libraries
	QMAKE_LFLAGS += -Wl,--start-group

	CONFIG(debug, debug|release) {
	} else {
		# Tell linker to use ASLR protection
		QMAKE_LFLAGS += -Wl,-dynamicbase
		# Tell linker to use DEP protection
		QMAKE_LFLAGS += -Wl,-nxcompat
	}

	for(lib, LIB_DIR):LIBS += -L"$$lib"
	LIBS += -lssl -lcrypto -lpthread -lminiupnpc -lz
	LIBS += -lcrypto -lws2_32 -lgdi32
	LIBS += -luuid -lole32 -liphlpapi -lcrypt32
	LIBS += -lole32 -lwinmm

	PROTOCPATH=$$BIN_DIR

	RC_FILE = resources/retroshare_win.rc

	DEFINES *= WINDOWS_SYS _USE_32BIT_TIME_T

	DEPENDPATH += . $$INC_DIR
	INCLUDEPATH += . $$INC_DIR
}

##################################### MacOS ######################################

macx {
    # ENABLE THIS OPTION FOR Univeral Binary BUILD.
    # CONFIG += ppc x86 

	LIBS += -Wl,-search_paths_first
        LIBS += -lssl -lcrypto -lz
	for(lib, LIB_DIR):exists($$lib/libminiupnpc.a){ LIBS += $$lib/libminiupnpc.a}
	LIBS += -framework CoreFoundation
	LIBS += -framework Security
	for(lib, LIB_DIR):LIBS += -L"$$lib"
	for(bin, BIN_DIR):LIBS += -L"$$bin"

	DEPENDPATH += . $$INC_DIR
	INCLUDEPATH += . $$INC_DIR

	sshserver {
		LIBS += -L../../../lib
		#LIBS += -L../../../lib/libssh-0.6.0
	}

    QMAKE_CXXFLAGS *= -Dfseeko64=fseeko -Dftello64=ftello -Dstat64=stat -Dstatvfs64=statvfs -Dfopen64=fopen

}

##################################### FreeBSD ######################################

freebsd-* {
	INCLUDEPATH *= /usr/local/include/gpgme
	LIBS *= -lssl
	LIBS *= -lgpgme
	LIBS *= -lupnp
	LIBS *= -lgnome-keyring
}

##################################### OpenBSD  ######################################

openbsd-* {
	INCLUDEPATH *= /usr/local/include
	QMAKE_CXXFLAGS *= -Dfseeko64=fseeko -Dftello64=ftello -Dstat64=stat -Dstatvfs64=statvfs -Dfopen64=fopen
	LIBS *= -lssl -lcrypto
	LIBS *= -lgpgme
	LIBS *= -lupnp
	LIBS *= -lgnome-keyring
	LIBS *= -rdynamic
}

##################################### Haiku ######################################

haiku-* {
	QMAKE_CXXFLAGS *= -D_BSD_SOURCE

	PRE_TARGETDEPS *= ../../libretroshare/src/lib/libretroshare.a
	PRE_TARGETDEPS *= ../../openpgpsdk/src/lib/libops.a

	LIBS *= ../../libretroshare/src/lib/libretroshare.a
	LIBS *= ../../openpgpsdk/src/lib/libops.a -lbz2 -lbsd
	LIBS *= -lssl -lcrypto -lnetwork
	LIBS *= -lgpgme
	LIBS *= -lupnp
	LIBS *= -lz
	LIBS *= -lixml

	LIBS += ../../supportlibs/pegmarkdown/lib/libpegmarkdown.a
	LIBS += -lsqlite3
	
}

############################## Common stuff ######################################

DEPENDPATH += . ../../libretroshare/src
INCLUDEPATH += . ../../libretroshare/src

PRE_TARGETDEPS *= ../../libretroshare/src/lib/libretroshare.a
LIBS *= ../../libretroshare/src/lib/libretroshare.a

# Input
HEADERS +=  notifytxt.h
SOURCES +=  notifytxt.cc \
            retroshare.cc

introserver {
	HEADERS += introserver.h
	SOURCES += introserver.cc
	DEFINES *= RS_INTRO_SERVER
}

webui {
	DEFINES *= ENABLE_WEBUI
        PRE_TARGETDEPS *= ../../libresapi/src/lib/libresapi.a
	LIBS += ../../libresapi/src/lib/libresapi.a
        DEPENDPATH += ../../libresapi/src
	INCLUDEPATH += ../../libresapi/src
        HEADERS += \
            TerminalApiClient.h
        SOURCES +=  \
            TerminalApiClient.cpp
}

sshserver {

	# This Requires libssh-0.5.* to compile.
	# Please use this path below.
        # (You can modify it locally if required - but dont commit it!)

	#LIBSSH_DIR = ../../../lib/libssh-0.5.2
	LIBSSH_DIR = ../../../libssh-0.6.0rc1

	#
	# Use the following commend to generate a Server RSA Key.
	# Key should be in current directory - when run/
	# ssh-keygen -t rsa -f rs_ssh_host_rsa_key
        #
        # You can connect from a standard ssh, eg: ssh -p 7022 127.0.0.1
 	#
	# The Menu system is available from the command-line (-T) and SSH (-S)
	# if it get covered by debug gunk, just press <return> to refresh.
	#
	# ./retroshare-nogui -h  provides some more instructions.
	#

	win32 {
		DEFINES *= LIBSSH_STATIC
	}

	DEPENDPATH += $$LIBSSH_DIR/include/
	INCLUDEPATH += $$LIBSSH_DIR/include/

	win32 {
		LIBS += -lssh
		LIBS += -lssh_threads
	} else {
		SSH_OK = $$system(pkg-config --atleast-version 0.5.4 libssh && echo yes)
		isEmpty(SSH_OK) {
			exists($$LIBSSH_DIR/build/src/libssh.a):exists($$LIBSSH_DIR/build/src/threads/libssh_threads.a) {
				LIBS += $$LIBSSH_DIR/build/src/libssh.a
				LIBS += $$LIBSSH_DIR/build/src/threads/libssh_threads.a
			} 
			else {
				! exists($$LIBSSH_DIR/build/src/libssh.a):message($$LIBSSH_DIR/build/src/libssh.a does not exist)
				! exists($$LIBSSH_DIR/build/src/threads/libssh_threads.a):message($$LIBSSH_DIR/build/src/threads/libssh_threads.a does not exist)
				message(You need to download and compile libssh)
				message(See http://sourceforge.net/p/retroshare/code/6163/tree/trunk/)
			}
		} else {
			LIBS += -lssh
			LIBS += -lssh_threads
		}
 	}

	HEADERS += ssh/rssshd.h
	SOURCES += ssh/rssshd.cc

	# For the Menu System
	HEADERS += menu/menu.h \
		menu/menus.h \
		menu/stdiocomms.h \

	SOURCES += menu/menu.cc \
		menu/menus.cc \
		menu/stdiocomms.cc \

	# For the RPC System
	HEADERS += rpc/rpc.h \
		rpc/rpcserver.h \
		rpc/rpcsetup.h \
		rpc/rpcecho.h \
		rpcsystem.h \

	SOURCES += rpc/rpc.cc \
		rpc/rpcserver.cc \
		rpc/rpcsetup.cc \
		rpc/rpcecho.cc \

	# Actual protocol files to go here...
	#HEADERS += rpc/proto/rpcecho.h \

	#SOURCES += rpc/proto/rpcecho.cc \

	DEFINES *= RS_SSH_SERVER

	# Include Protobuf classes.
	CONFIG += protorpc
}

protorpc {
	# Proto Services
	PROTOS = core.proto peers.proto system.proto chat.proto search.proto files.proto stream.proto
	DESTPATH = $$PWD/rpc/proto/gencc
	PROTOPATH = $$PWD/../../rsctrl/src/definition
	CMD = echo Building protobuf files
	for(pf, PROTOS):CMD += && $${PROTOCPATH}protoc --cpp_out=$${DESTPATH} --proto_path=$${PROTOPATH} $${PROTOPATH}/$${pf}
	protobuf_gen.commands = $${CMD}
	QMAKE_EXTRA_TARGETS += protobuf_gen
	PRE_TARGETDEPS += protobuf_gen

	HEADERS += rpc/proto/rpcprotopeers.h \
		rpc/proto/rpcprotosystem.h \
		rpc/proto/rpcprotochat.h \
		rpc/proto/rpcprotosearch.h \
		rpc/proto/rpcprotofiles.h \
		rpc/proto/rpcprotostream.h \
		rpc/proto/rpcprotoutils.h \

	SOURCES += rpc/proto/rpcprotopeers.cc \
		rpc/proto/rpcprotosystem.cc \
		rpc/proto/rpcprotochat.cc \
		rpc/proto/rpcprotosearch.cc \
		rpc/proto/rpcprotofiles.cc \
		rpc/proto/rpcprotostream.cc \
		rpc/proto/rpcprotoutils.cc \

	# Offical Generated Code (protobuf 2.4.1)
	HEADERS += rpc/proto/gencc/core.pb.h \
		        rpc/proto/gencc/peers.pb.h \
		        rpc/proto/gencc/system.pb.h \
		        rpc/proto/gencc/chat.pb.h \
        		rpc/proto/gencc/search.pb.h \
		        rpc/proto/gencc/files.pb.h \
		        rpc/proto/gencc/stream.pb.h \

	SOURCES += rpc/proto/gencc/core.pb.cc \
		        rpc/proto/gencc/peers.pb.cc \
		        rpc/proto/gencc/system.pb.cc \
		        rpc/proto/gencc/chat.pb.cc \
		        rpc/proto/gencc/search.pb.cc \
		        rpc/proto/gencc/files.pb.cc \
		        rpc/proto/gencc/stream.pb.cc \

	# Generated ProtoBuf Code the RPC System
        # If you are developing, or have a different version of protobuf
        # you can use these ones (run make inside rsctrl/src/ to generate)
	#HEADERS += ../../rsctrl/src/gencc/core.pb.h \
	#	        ../../rsctrl/src/gencc/peers.pb.h \
	#	        ../../rsctrl/src/gencc/system.pb.h \
	#	        ../../rsctrl/src/gencc/chat.pb.h \
        #		../../rsctrl/src/gencc/search.pb.h \
	#	        ../../rsctrl/src/gencc/files.pb.h \
	#	        ../../rsctrl/src/gencc/stream.pb.h \

	#SOURCES += ../../rsctrl/src/gencc/core.pb.cc \
	#	        ../../rsctrl/src/gencc/peers.pb.cc \
	#	        ../../rsctrl/src/gencc/system.pb.cc \
	#	        ../../rsctrl/src/gencc/chat.pb.cc \
	#	        ../../rsctrl/src/gencc/search.pb.cc \
	#	        ../../rsctrl/src/gencc/files.pb.cc \
	#	        ../../rsctrl/src/gencc/stream.pb.cc \

	DEPENDPATH *= rpc/proto/gencc
	INCLUDEPATH *= rpc/proto/gencc

	!win32 {
		# unrecognized option
		QMAKE_CFLAGS += -pthread
		QMAKE_CXXFLAGS += -pthread
	}
	LIBS += -lprotobuf -lpthread
	
	win32 {
		DEPENDPATH += $$LIBS_DIR/include/protobuf
		INCLUDEPATH += $$LIBS_DIR/include/protobuf
	}
	
	macx {
		PROTOPATH = ../../../protobuf-2.4.1
		INCLUDEPATH += $${PROTOPATH}/src
	}
}
