/*******************************************************************************
 * gui/PluginManager.cpp                                                       *
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

#include <QFile>
#include <QDir>
#include <QRegExp>

#include <QDebug>

#include <QWidget> //a strange thing: it compiles without this header, but 
                  //then segfaults in some place

#include <QApplication>

#include "PluginManager.h"
#include "PluginManagerWidget.h"
#include "plugins/PluginInterface.h"
#include "rshare.h"

//=============================================================================
//=============================================================================
//=============================================================================

PluginManager::PluginManager()
{
    baseFolder =  //qApp->applicationDirPath()+"///plugins" ;
RsApplication::dataDirectory() + "/plugins" ;
    lastError = "No error.";

    viewWidget = 0;

//    defaultLoad();
}

//=============================================================================
      
void      
PluginManager::defaultLoad(  )
{   
   qDebug() << "  " << "Default load started" ;

    QDir workDir(baseFolder);

    if ( !workDir.exists() )
    {
        QString em= tr("base folder %1 doesn't exist, default load failed")
	              .arg( baseFolder );
        emit errorAppeared( em ); 
	return ;
    }

  //=== get current available plugins =====
    QStringList currAvailable = workDir.entryList(QDir::Files);
#if defined(Q_OS_WIN)
    QRegExp trx("*.dll") ;
#elif defined(__MACH__)
    QRegExp trx("*.dylib");
#else
    QRegExp trx("*.so");
#endif
    trx.setPatternSyntax(QRegExp::Wildcard );

    currAvailable.filter( trx );

    qDebug() << "  " << "can load this plugins: " << currAvailable ;

  //=== 
    foreach(QString pluginFileName, currAvailable)
    {
       QString fullfn( workDir.absoluteFilePath( pluginFileName ) );
       QString newName;
       int ti = readPluginInformation( fullfn, newName);
       if (! ti )
       {
           acceptPlugin(fullfn, newName);
       }
    }//    foreach(QString pluginFileName, currAvailable)

    qDebug() << "  " << "names are " << names;
}

//=============================================================================

int
PluginManager::readPluginInformation(QString fullFileName, QString& pluginName)
{
    qDebug() << "  " << "processing file " << fullFileName;

    PluginInterface* plugin = loadPluginInterface(fullFileName) ;
    pluginName = "Undefined name" ;
    if (plugin)
    {
        pluginName = plugin->pluginName();
        qDebug() << "  " << "got pluginName:"  << pluginName;
        delete plugin;
        return 0 ;
    }
    else
    {
       //do not emit anything, cuz error message already was sent 
      //from loadPluginInterface(..)
        return 1; //this means, some rrror appeared
    }
}

//=============================================================================

PluginInterface* 
PluginManager::loadPluginInterface(QString fileName)
{
    QString errorMessage = "Default Error Message" ;
    PluginInterface* plugin = 0 ;
    QPluginLoader* plLoader = new QPluginLoader(fileName);

    QObject *pluginObject = plLoader->instance();
    if (pluginObject)
    {
        //qDebug() << "  " << "loaded..." ;
        plugin = qobject_cast<PluginInterface*> (pluginObject) ;
	   
        if (plugin)
        {
            errorMessage = "No error" ;            
        }
        else
        {
            errorMessage = "Cast to 'PluginInterface*' failed";         
            emit errorAppeared( errorMessage );
        }        
    }
    else
    {
        errorMessage = "Istance wasn't created: " + plLoader->errorString() ;
        emit errorAppeared( errorMessage );
    }

    delete plLoader; // plugin instance will not be deleted with this action
    return plugin;
}

//=============================================================================

PluginManager:: ~PluginManager()
{   
}

//=============================================================================

void 
PluginManager::acceptPlugin(QString fileName, QString pluginName)
{
    qDebug() << "  " << "accepting plugin " << pluginName;

    names.append(pluginName);
    fileNames.append(fileName);

    if (viewWidget)
       viewWidget->registerNewPlugin( pluginName );

    emit newPluginRegistered( pluginName );
}

//=============================================================================

QWidget* 
PluginManager::pluginWidget(QString pluginName)
{
    QWidget* result = 0;
    int plIndex = names.indexOf( pluginName ) ;
    if (plIndex >=0 )
    {
    //=== load plugin's interface
        QString fn = fileNames.at(plIndex);
        PluginInterface* pliface = loadPluginInterface(fn);
        if (pliface)
        {
       //=== now, get a widget
            result = pliface->pluginWidget() ;
            if (result)
            {
             // all was good,
                qDebug() << "  " << "got plg widget..." ;
		return result;
            }
            else
            {
                QString em=tr("Error: instance '%1' can't create a widget")
                            .arg( pluginName );
                emit errorAppeared( em );
		return 0;
            }
        }
        else
        {
            // do nothing here...
        }
    }
    else
    {
        QString em = tr("Error: no plugin with name '%1' found")
                        .arg(pluginName);
        emit errorAppeared( em );
    }      
}

//=============================================================================

QWidget* 
PluginManager::getViewWidget(QWidget* parent )
{
    if (viewWidget)
        return viewWidget;
    
  //=== else, create the viewWidget and return it
  //
    viewWidget = new PluginManagerWidget();
    
    foreach(QString pn, names)
    {
       qDebug() << "  " << "reg new plg " << pn;
       viewWidget->registerNewPlugin( pn );
    }
    
    connect(this      , SIGNAL( errorAppeared(QString) )    ,
            viewWidget, SLOT( acceptErrorMessage( QString)));
    
    connect(viewWidget, SIGNAL( destroyed() )    ,
            this      , SLOT( viewWidgetDestroyed( )));

    connect(viewWidget, SIGNAL( installPluginRequested(QString)),
	    this      , SLOT(   installPlugin( QString)));

    connect(viewWidget, SIGNAL( removeRequested( QString ) ),
	    this      , SLOT(   removePlugin(QString )));
	    
    


    qDebug() << "  PluginManager::getViewWidget done";

    return viewWidget;
}

//=============================================================================

void
PluginManager::viewWidgetDestroyed(QObject* obj )
{ 
    qDebug() << "  PluginManager::viewWidgetDestroyed is here";
    
    viewWidget = 0;
}

//=============================================================================

void 
PluginManager::removePlugin(QString pluginName)
{
    QWidget* result = 0;
    int plIndex = names.indexOf( pluginName ) ;
    if (plIndex >=0 )
    {
        QString fn = fileNames.at(plIndex);
        if (QDir::isRelativePath(fn))
	    fn = QDir(baseFolder).path() + QDir::separator() + fn ;

        QFile fl(fn);
        if (!fl.remove())
        {
	    QString em = tr("Error: failed to remove file %1"
	                         " (uninstalling plugin '%2')")
                            .arg(fn).arg(pluginName);
            emit errorAppeared( em);
        }
        else
        {
            if (viewWidget)
	        viewWidget->removePluginFrame( pluginName);
	}
    }
    else
    {
       QString em = tr("Error (uninstall): no plugin with name '%1' found")
		      .arg(pluginName);
       emit errorAppeared( em );
    }
}

//=============================================================================

void
PluginManager::installPlugin(QString fileName)
{
    qDebug() << "  " << "PluginManager::installPlugin is here" ;
   
    if (!QFile::exists( fileName) )
    {       
       QString em = tr("Error (installation): plugin file %1 doesn't exist")
 		      .arg( fileName );

       emit errorAppeared( em );
       return ;
    }

    QString pn;
    if (! readPluginInformation(fileName, pn))
    {
        QFile sf( fileName) ;
        QString newFileName = baseFolder + QDir::separator() +
	                      QFileInfo( fileName).fileName();
        if ( QFile::copy( fileName, newFileName ) )
        {
            QString pn;
            int ti = readPluginInformation( newFileName , pn );
            if (! ti )
            {
                acceptPlugin(newFileName, pn);
            }
        } //    
	else
	{
	    QString em = tr("Error: can't copy %1 to %2")
	                    .arg(fileName, newFileName) ;
            emit errorAppeared( em );
	}
    }
    else
    {
//       do nothing here: read plugin information emits its own error
    }
}

//=============================================================================

