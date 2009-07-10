INCLUDEPATH += $$PWD \
    ../$$PWP
DEPENDPATH += $$PWD
HEADERS = $$PWP/p3face.h \
        $$PWP/p3blog.h \
        $$PWP/p3discovery.h

SOURCES += p3face.cc \
        $$PWP/p3blog.cc \
        $$PWP/p3discovery.cc
