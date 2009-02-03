#=== this part is common (similar) for all plugin projects =====================
TEMPLATE      = lib
CONFIG       += plugin debug

# this is directory, where PluginInterface.h is located
INCLUDEPATH  += ../

# and, the result (*.so or *.dll) should appear in this directory
DESTDIR       = ../bin
OBJECTS_DIR = temp/obj
RCC_DIR = temp/qrc
UI_DIR  = temp/ui
MOC_DIR = temp/moc


# the name of the result file; 
TARGET        = $$qtLibraryTarget(calendar_plugin)

HEADERS     += ../PluginInterface.h  \
               src/CalendarPlugin.h
SOURCES     += src/CalendarPlugin.cpp
                
#===============================================================================

#=== and this are definitions, specific for this program =======================
HEADERS    += src/mainwindow.h
SOURCES    += src/mainwindow.cpp
#===============================================================================
