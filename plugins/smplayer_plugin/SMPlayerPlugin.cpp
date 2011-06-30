//#include <QApplication>
//#include <QString>
#include <QVBoxLayout>

#include "SMPlayerPlugin.h"
#include "smplayer.h"

SMPlayerPluginWidget::SMPlayerPluginWidget(QWidget* parent,
                                           Qt::WindowFlags flags  )
                     :QFrame(parent)                     
{
    player = new SMPlayer();

    lay = new QVBoxLayout(this);
    lay->addWidget( player->gui() );
}

SMPlayerPluginWidget::~SMPlayerPluginWidget()
{
    delete player;
}


//==============================================================================

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
    SMPlayerPluginWidget* wd = new SMPlayerPluginWidget(parent);
    return wd ;
}


Q_EXPORT_PLUGIN2(smplayer_plugin, SMPlayerPlugin)
