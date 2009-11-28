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


SOURCES += main.cpp \
    mainwindow.cpp \
    colorpeg.cpp \
    rowhint.cpp \
    pegrow.cpp \
    msg.cpp \
    about.cpp
HEADERS += mainwindow.h \
    colorpeg.h \
    rowhint.h \
    pegrow.h \
    msg.h \
    about.h
RESOURCES += resource.qrc
FORMS += about.ui
OTHER_FILES += docs/GPL.html \
    trans_en.ts \
    trans_de.ts
TRANSLATIONS = trans_en.ts \
    trans_de.ts
CODECFORTR = UTF-8
CODECFORSRC = UTF-8
