INCLUDEPATH += $$PWD \
    ../$$PWP
DEPENDPATH += $$PWD

HEADERS = $$PWP/p3face.h \
    $$PWP/p3blog.h \
    $$PWP/p3discovery.h \
    rsserver.h \
    p3files.h \
    p3msgs.h \
    p3peers.h \
    p3photo.h \
    p3rank.h
SOURCES += $$PWP/p3blog.cc \
    $$PWP/p3discovery.cc \
    $$PWP/p3face-config.cc \
    rsserver.cc \
    p3face-msgs.cc \
    p3face-server.cc \
    p3face-startup.cc \
    p3msgs.cc \
    p3peers.cc \
    p3photo.cc \
    p3rank.cc \
    testrschanid.cc

