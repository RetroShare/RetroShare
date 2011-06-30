#ifndef _HWA_PLUGIN_H_
#define _HWA_PLUGIN_H_

#include <QObject>

#include <QString>
#include <QWidget>
#include <QFrame>
class QVBoxLayout;

#include <PluginInterface.h>

#include <QDebug>

class SMPlayer;

class SMPlayerPluginWidget: public QFrame
{
    Q_OBJECT

    public:
        SMPlayerPluginWidget(QWidget* parent,  Qt::WindowFlags flags = 0 );
        ~SMPlayerPluginWidget();

    protected:
        SMPlayer* player;        
        QVBoxLayout* lay;
};

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
