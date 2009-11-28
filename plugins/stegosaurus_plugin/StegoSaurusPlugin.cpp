//#include <QApplication>
//#include <QString>
//#include <QPushButton>

#include "StegoSaurusPlugin.h"
#include "stegosaurus.h"

QString
StegoSaurusPlugin::pluginDescription() const
{
    QString res;
    res = "a StegoSaurus plugin" ;

    return res; 
}

QString
StegoSaurusPlugin::pluginName() const
{
    return "StegoSaurus" ;
}

QWidget*
StegoSaurusPlugin::pluginWidget(QWidget * parent )
{
    StegoSaurus* window = new StegoSaurus(parent);

    return window;
}


Q_EXPORT_PLUGIN2(stegosaurus_plugin, StegoSaurusPlugin)
