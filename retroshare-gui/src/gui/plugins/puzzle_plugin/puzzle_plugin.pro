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
TARGET        = $$qtLibraryTarget(puzzle_plugin)

HEADERS     += ../PluginInterface.h  \
               src/PuzzlePlugin.h
SOURCES     += src/PuzzlePlugin.cpp
                
#===============================================================================



#=== and here are definitions, specific for this program =======================
HEADERS    += src/mainwindow.h         \
              src/pieceslist.h         \
              src/puzzlewidget.h

RESOURCES   = puzzle.qrc

SOURCES    += src/mainwindow.cpp       \
              src/pieceslist.cpp       \
              src/puzzlewidget.cpp
#===============================================================================
