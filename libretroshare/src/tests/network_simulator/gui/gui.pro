TEMPLATE = app

CONFIG *= qt qglviewer uic
QT *= xml opengl

INCLUDEPATH *= ../../.. ..

TARGET = NetworkSim
DESTDIR = ../bin

PRE_TARGETDEPS = ../nscore/nscore.pro

SOURCES = main.cpp NetworkViewer.cpp NetworkSimulatorGUI.cpp \
          TurtleRouterStatistics.cpp RsAutoUpdatePage.cpp GlobalRouterStatistics.cpp

HEADERS = NetworkViewer.h NetworkSimulatorGUI.h \
          TurtleRouterStatistics.h RsAutoUpdatePage.h  GlobalRouterStatistics.h

FORMS = NetworkSimulatorGUI.ui TurtleRouterStatistics.ui GlobalRouterStatistics.ui

LIBS *= ../../../lib/libretroshare.a \
        ../../../../../libbitdht/src/lib/libbitdht.a \
		  ../../../../../openpgpsdk/src/lib/libops.a \
		  ../lib/libnscore.a \
		  -lsqlcipher -lgnome-keyring -lupnp -lssl -lcrypto -lbz2 -lixml
