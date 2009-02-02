#ifndef _HWA_PLUGIN_H_
#define _HWA_PLUGIN_H_

#include <QObject>

#include <QString>
#include <QWidget>

#include <PluginInterface.h>

#include <QDebug>

class SMPlayer;


class SMPlayerPlugin: public QObject, public PluginInterface
{
    Q_OBJECT
    Q_INTERFACES(PluginInterface)

    public slots:
            
        virtual QString pluginDescription() const ;
        virtual QString pluginName() const ;

        virtual QWidget* pluginWidget(QWidget * parent = 0) ;

};

#endif 
