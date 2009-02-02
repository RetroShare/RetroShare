//#include <QApplication>
//#include <QString>
//#include <QPushButton>

#include "DiagramPlugin.h"
#include "mainwindow.h"

QString
DiagramPlugin::pluginDescription() const
{
    QString res;
    res = "A qdiagram plugin" ;

    return res; 
}

QString
DiagramPlugin::pluginName() const
{
    return "QDiagram" ;
}

QWidget*
DiagramPlugin::pluginWidget(QWidget * parent )
{
    MainWindow* window = new MainWindow(parent);
    //window->openImage(":/images/example.jpg");

    return window;
}


Q_EXPORT_PLUGIN2(qdiagram_plugin, DiagramPlugin)
