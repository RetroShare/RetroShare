TEMPLATE = lib
CONFIG *= staticlib

INCLUDEPATH *= ../../.. ..

TARGET = nscore

SOURCES = Network.cpp \
			 PeerNode.cpp \
          MonitoredRsPeers.cpp \
			 MonitoredTurtle.cpp

HEADERS = Network.h \
			 PeerNode.h \
			 MonitoredTurtle.h  \
          MonitoredRsPeers.h

DESTDIR = ../lib
