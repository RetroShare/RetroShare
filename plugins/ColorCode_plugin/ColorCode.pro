# -------------------------------------------------
# Project created by QtCreator 2009-06-24T21:25:25
# -------------------------------------------------
#=== this part is common (similar) for all plugin projects =====================
TEMPLATE      = lib
CONFIG       += plugin release

# this is directory, where PluginInterface.h is located
INCLUDEPATH  += ../

# and, the result (*.so or *.dll) should appear in this directory
DESTDIR       = ../bin
OBJECTS_DIR = temp/obj
RCC_DIR = temp/qrc
UI_DIR  = temp/ui
MOC_DIR = temp/moc


# the name of the result file; 
TARGET        = $$qtLibraryTarget(colorcode_plugin)

HEADERS     += ../PluginInterface.h  \
               ColorCodePlugin.h
SOURCES     += ColorCodePlugin.cpp
                
#===============================================================================

# Input
HEADERS += about.h \
    colorcode.h \
    colorpeg.h \
    msg.h \
    pegrow.h \
    rowhint.h \
    ccsolver.h \
    background.h \
    solrow.h
FORMS += about.ui
SOURCES += about.cpp \
    colorcode.cpp \
    colorpeg.cpp \
    main.cpp \
    msg.cpp \
    pegrow.cpp \
    rowhint.cpp \
    ccsolver.cpp \
    background.cpp \
    solrow.cpp
RESOURCES += resource.qrc
OTHER_FILES += docs/GPL.html
win32 {
    RC_FILE = ColorCode.rc
}
TRANSLATIONS += trans_de.ts \
    trans_en.ts \
    trans_cs.ts
CODECFORTR = UTF-8
CODECFORSRC = UTF-8
