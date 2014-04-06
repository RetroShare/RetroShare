TEMPLATE = app

CONFIG *= qt qglviewer 
QT *= xml opengl

INCLUDEPATH *= ../../.. ..

TARGET = NetworkSim
DESTDIR = bin

PRE_TARGETDEPS = ../nscore/nscore.pro

SOURCES = main.cpp NetworkViewer.cpp NetworkSimulatorGUI.cpp \
          TurtleRouterStatistics.cpp RsAutoUpdatePage.cpp 

HEADERS = NetworkViewer.h NetworkSimulatorGUI.h \
          TurtleRouterStatistics.h RsAutoUpdatePage.h 

FORMS = NetworkSimulatorGUI.ui TurtleRouterStatistics.ui

LIBS *= ../../../lib/libretroshare.a \
        ../../../../../libbitdht/src/lib/libbitdht.a \
		  ../../../../../../lib/sqlcipher/.libs/libsqlcipher.a   \
		  ../../../../../openpgpsdk/src/lib/libops.a \
		  ../lib/libnscore.a \
		  -lgnome-keyring -lupnp -lssl -lcrypto -lbz2 -lixml
