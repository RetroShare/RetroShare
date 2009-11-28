//#include <QApplication>
//#include <QString>
//#include <QPushButton>

#include "I2PMessengerPlugin.h"
#include "gui/form_Main.h"

QString
I2PMessengerPlugin::pluginDescription() const
{
    QString res;
    res = "A I2P Messenger plugin" ;

    return res; 
}

QString
I2PMessengerPlugin::pluginName() const
{
    return "I2P Messenger" ;
}

QWidget*
I2PMessengerPlugin::pluginWidget(QWidget * parent )
{
      form_MainWindow* mainForm= new form_MainWindow();
	    return mainForm;
}


Q_EXPORT_PLUGIN2(i2pmessenger_plugin, I2PMessengerPlugin)
