#ifndef _PLUGIN_INTERFACE_H_
#define _PLUGIN_INTERFACE_H_

#include <QtPlugin>

QT_BEGIN_NAMESPACE
class QString;
class QWidget;
QT_END_NAMESPACE

//! a base class for plugins

//! All plugin classes must inherite this class and QObject. 
class PluginInterface
{
public:
    virtual ~PluginInterface() {}

public slots:
    //! A description of the plugin

    //! A description of the plugin. Is not used in current version. 
    virtual QString pluginDescription() const = 0;

    //! The plugin's name

    //! A name serves like an unique ID. The name is used in all operations
    //! such as installing, removing, receiving a widget
    virtual QString pluginName() const = 0;

    //! plugin's widget.

    //! Returns the widget, which is actually a plugin. Main application must
    //! delete the widget; usually, a parent widget does it. If you want to use
    //! the widget  as top-level (i.e. parent==0), please, set
    //! Qt::WA_DeleteOnClose  flag .
    virtual QWidget* pluginWidget(QWidget * parent = 0) = 0;
};

QT_BEGIN_NAMESPACE

Q_DECLARE_INTERFACE(PluginInterface,
                    "com.beardog.retroshare.PluginInterface/1.0")

QT_END_NAMESPACE

#endif
 
