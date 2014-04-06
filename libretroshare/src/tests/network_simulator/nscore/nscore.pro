TEMPLATE = lib
CONFIG *= staticlib

INCLUDEPATH *= ../../.. ..

TARGET = nscore

SOURCES = Network.cpp \
			 PeerNode.cpp \
          MonitoredRsPeers.cpp \
			 MonitoredTurtleClient.cpp \
			 MonitoredGRouterClient.cpp

HEADERS = Network.h \
			 PeerNode.h \
          MonitoredRsPeers.h \
			 MonitoredTurtleClient.h  \
			 MonitoredGRouterClient.h

DESTDIR = ../lib
