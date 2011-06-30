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
TARGET        = $$qtLibraryTarget(stegosaurus_plugin)

HEADERS     += ../PluginInterface.h  \
               StegoSaurusPlugin.h
SOURCES     += StegoSaurusPlugin.cpp
                
#===============================================================================

# Input
HEADERS += resource.h resource1.h stegosaurus.h
FORMS += stegosaurus.ui
SOURCES += main.cpp stegosaurus.cpp
RESOURCES += stegosaurus.qrc
