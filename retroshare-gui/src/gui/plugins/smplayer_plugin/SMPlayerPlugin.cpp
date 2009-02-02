//#include <QApplication>
//#include <QString>
//#include <QPushButton>

#include "SMPlayerPlugin.h"
#include "smplayer.h"

QString
SMPlayerPlugin::pluginDescription() const
{
    QString res;
    res = "A SMPlayer plugin" ;

    return res; 
}

QString
SMPlayerPlugin::pluginName() const
{
    return "SMPlayer" ;
}

QWidget*
SMPlayerPlugin::pluginWidget(QWidget * parent )
{
    	SMPlayer *smplayer = new SMPlayer();
    	smplayer->start();
        //return smplayer;
}


Q_EXPORT_PLUGIN2(smplayer_plugin, SMPlayerPlugin)
