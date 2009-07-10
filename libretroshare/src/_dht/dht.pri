INCLUDEPATH += $$PWD \
    ../$$PWP
DEPENDPATH += $$PWD
SOURCES = b64.cc \
    dht_bootstrap.cc \
    dht_check_peers.cc \
    dhthandler.cc \
    opendht.cc \
    opendhtmgr.cc
HEADERS = b64.h \
    dht_bootstrap.h \
    dhtclient.h \
    dht_check_peers.h \
    dhthandler.h \
    opendht.h \
    opendhtmgr.h
