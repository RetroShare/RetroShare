TEMPLATE = app

CONFIG *= qt qglviewer 
QT *= xml opengl

INCLUDEPATH *= ../.. ..

TARGET = NetworkSim

SOURCES = Network.cpp  main.cpp NetworkViewer.cpp NetworkSimulatorGUI.cpp \
          TurtleRouterStatistics.cpp RsAutoUpdatePage.cpp MonitoredRsPeers.cpp \
			 MonitoredTurtle.cpp

HEADERS = Network.h   MonitoredTurtle.h NetworkViewer.h NetworkSimulatorGUI.h \
          TurtleRouterStatistics.h RsAutoUpdatePage.h MonitoredRsPeers.h

FORMS = NetworkSimulatorGUI.ui TurtleRouterStatistics.ui

LIBS *= ../../lib/libretroshare.a ../../../../libbitdht/src/lib/libbitdht.a ../../../../../lib/sqlcipher/.libs/libsqlcipher.a   ../../../../openpgpsdk/src/lib/libops.a -lgnome-keyring -lupnp -lssl -lcrypto -lbz2 -lixml
