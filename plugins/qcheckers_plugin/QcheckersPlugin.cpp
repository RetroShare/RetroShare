//#include <QApplication>
//#include <QString>
//#include <QPushButton>

#include "QcheckersPlugin.h"
#include "toplevel.h"

QString
QcheckersPlugin::pluginDescription() const
{
    QString res;
    res = "A QCheckers plugin" ;

    return res; 
}

QString
QcheckersPlugin::pluginName() const
{
    return "QCheckers" ;
}

QWidget*
QcheckersPlugin::pluginWidget(QWidget * parent )
{
    myTopLevel* top = new myTopLevel();

    return top;

}


Q_EXPORT_PLUGIN2(qcheckers_plugin, QcheckersPlugin)
