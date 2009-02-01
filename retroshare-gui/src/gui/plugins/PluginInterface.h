#ifndef _PLUGIN_INTERFACE_H_
#define _PLUGIN_INTERFACE_H_

#include <QtPlugin>

QT_BEGIN_NAMESPACE
class QString;
class QWidget;
QT_END_NAMESPACE

//
class PluginInterface
{
public:
    virtual ~PluginInterface() {}

public slots:
    virtual QString pluginDescription() const = 0;
    virtual QString pluginName() const = 0;

    virtual QWidget* pluginWidget(QWidget * parent = 0) = 0;
};

QT_BEGIN_NAMESPACE

Q_DECLARE_INTERFACE(PluginInterface,
                    "com.beardog.retroshare.PluginInterface/1.0")

QT_END_NAMESPACE

#endif
 
