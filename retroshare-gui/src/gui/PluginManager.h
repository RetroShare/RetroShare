/*******************************************************************************
 * gui/PluginManager.h                                                         *
 *                                                                             *
 * Copyright (c) 2006 Retroshare Team  <retroshare.project@gmail.com>          *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Affero General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/

#ifndef _PLUGIN_MANAGER_H_
#define _PLUGIN_MANAGER_H_

#include <QObject>

#include <QString>
#include <QStringList>
#include <QList>

#include <QPluginLoader>

#include <QVector>

class PluginInterface;
class PluginManagerWidget;

//! An engine for plugins management

//!     This class performs oaa plugin management operations: installing,
//! loading, remowing. it also provides a PluginManagerWidget for controlling
//! itself. I supose, a appication has to create a global instance of the class,
//! so all pages (instances of the MainPage class) could receive plugin widgets.
class PluginManager: public QObject
{
    Q_OBJECT

public:
    PluginManager();
    ~PluginManager();

    //! Checks up 'plugins' folder for loadable plugins

    //!     This is a separate method, becouse an application should create
    //! a PluginManager instance, then connect all its signals, optionally
    //! create a view widget, and only after all perform lookup
    void defaultLoad(  ) ;

    //! GUI for the loader.

    //! Returns a PluginManagerWidget instance. When called for the first time,
    //! creates a new object; after that returns pointer to the same instance.
    //! After the instance was deleteted (it could be safely done in usual way)
    //! may create a new one.
    QWidget* getViewWidget(QWidget* parent = 0);

    //! Loads a widget of the plugin with given name

    //!   Loads a new copy (called twice will return different objects) of the
    //! plugin widget. If there is no plugin with given name, returns 0 (also
    //! emits errorAppeared(..) with an error description).
    //! PluginManager provides ablolutely no control over returned widget;
    //! the application should delete it, like all other widgets
    QWidget* pluginWidget(QString pluginName);
        
    //! returns last error appeared;

    //! Sorry, doesn't work in current implementation
    QString getLastError();

public slots:
    //! processes the desctucrion of the view widget;
    void viewWidgetDestroyed(QObject * obj = 0);

    //! processes the plugin installation request

    //!   After successful installatio a newPluginRegistered(..) signal will be
    //! emitted. On some error -- errorAppeared(..) will be emitted;
    //! 'Installation' means that plgin file (dll or so) will be checked and
    //! copied to the 'plugins' directory. Later, in pluginWidget(..) call this
    //! copy will be used
    void installPlugin(QString fileName);

    //! Processes plugin remove request

    //!   'Remove' means that plugin file (so or dll) will be physically deleted
    //! from 'plugins' folder. 
    void removePlugin(QString pluginName);

signals:
    //! PluginManager emits this signal on every error;

    //! This signal is connected to the PluginManagerWidget::acceptErrorMessage.
    //! So, all error messages will appear on the view widget (only if that one
    //! was created, of course)
    void errorAppeared(QString errorDescription);

    //! Is emitted after plugin removing;

    //!   Already loaded plugin widgets, will not be deleted. Nobody will touch
    //! them
    void pluginRemoveCompleted(QString pluginName);

    //! Is emitted for every loadable plugin

    void newPluginRegistered(QString pluginName);

protected:
    PluginManagerWidget* viewWidget;
    
    QStringList fileNames;
    QStringList names;

    QString baseFolder;
    QString lastError;

    //! Reads information from plugin file

    //!    The function tries to read the info from the plugin file (in current
    //! implementation only pluginName).
    //! \returns 0 on success, error code (>0) on fail
    int readPluginInformation( QString fileName, QString& pluginName);

    //! ---

    //!     Adds plugin name and plugin filename to the lists, emits nesessary
    //! signals, updates  the view widget
    void acceptPlugin( QString fileName, QString pluginName);


    PluginInterface* loadPluginInterface(QString fileName ) ;

}; 


#endif


