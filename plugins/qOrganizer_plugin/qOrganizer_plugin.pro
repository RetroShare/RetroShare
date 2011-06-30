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
TARGET        = $$qtLibraryTarget(qorganizer_plugin)

HEADERS     += ../PluginInterface.h  \
               qorganizerPlugin.h
SOURCES     += qorganizerPlugin.cpp
                
#===============================================================================

TARGET = 
DEPENDPATH += . lang
INCLUDEPATH += .

# Input
HEADERS += qorganizer.h settings.h delegates.h
SOURCES += main.cpp qorganizer.cpp settings.cpp delegates.cpp
RESOURCES += application.qrc
QT        += network sql
TRANSLATIONS += lang/Hungarian.ts \
                lang/Romanian.ts \
                lang/Portuguese.ts \
                lang/Slovenian.ts \
                lang/Russian.ts \
                lang/Spanish.ts \
                lang/Albanian.ts \
                lang/Macedonian.ts \
                lang/Estonian.ts \
		lang/Dutch.ts \
		lang/German.ts \
                lang/French.ts \
		lang/Polish.ts
