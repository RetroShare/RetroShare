//#include <QApplication>
//#include <QString>
//#include <QPushButton>

#include "LinksPlugin.h"
#include "LinksDialog.h"

QString
LinksPlugin::pluginDescription() const
{
    QString res;
    res = "Links Cloud plugin" ;

    return res; 
}

QString
LinksPlugin::pluginName() const
{
    return "Links Cloud" ;
}

QWidget*
LinksPlugin::pluginWidget(QWidget * parent )
{
    LinksDialog* window = new LinksDialog(parent);

    return window;
}


Q_EXPORT_PLUGIN2(linkscloud_plugin, LinksPlugin)
