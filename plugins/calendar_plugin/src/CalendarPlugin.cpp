//#include <QApplication>
//#include <QString>
//#include <QPushButton>

#include "CalendarPlugin.h"
#include "mainwindow.h"

QString
CalendarPlugin::pluginDescription() const
{
    QString res;
    res = "A simple plugin, based on QT calendar(richtext) example" ;

    return res; 
}

QString
CalendarPlugin::pluginName() const
{
    return "Calendar" ;
}

QWidget*
CalendarPlugin::pluginWidget(QWidget * parent )
{
    MainWindow* window = new MainWindow(parent);
    //window->openImage(":/images/example.jpg");

    return window;
}


Q_EXPORT_PLUGIN2(calendar_plugin, CalendarPlugin)
