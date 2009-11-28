//#include <QApplication>
//#include <QString>
//#include <QPushButton>

#include "PuzzlePlugin.h"
#include "mainwindow.h"

QString
PuzzlePlugin::pluginDescription() const
{
    QString res;
    res = "A simple plugin, based on QT puzzle example" ;

    return res; 
}

QString
PuzzlePlugin::pluginName() const
{
    return "Puzzle" ;
}

QWidget*
PuzzlePlugin::pluginWidget(QWidget * parent )
{
    MainWindow* window = new MainWindow(parent);
    window->openImage(":/images/example.jpg");

    return window;
}


Q_EXPORT_PLUGIN2(puzzle_plugin, PuzzlePlugin)
