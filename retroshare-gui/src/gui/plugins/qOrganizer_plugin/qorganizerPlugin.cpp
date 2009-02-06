//#include <QApplication>
//#include <QString>
//#include <QPushButton>

#include "qorganizerPlugin.h"
#include "qorganizer.h"

QString
qorganizerPlugin::pluginDescription() const
{
    QString res;
    res = "A qOrganizer plugin" ;

    return res; 
}

QString
qorganizerPlugin::pluginName() const
{
    return "Organizer" ;
}

QWidget*
qorganizerPlugin::pluginWidget(QWidget * parent )
{
    qOrganizer* window = new qOrganizer();
    //window->openImage(":/images/example.jpg");

    return window;
}


Q_EXPORT_PLUGIN2(qorganizer_plugin, qorganizerPlugin)
