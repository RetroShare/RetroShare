#include <QFile>
#include <QDir>
#include <QRegExp>

#include <QDebug>

#include <QWidget> //a strange thing: it compiles without this header, but 
                  //then segfaults in some place

#include "PluginManager.h"
#include "plugins/PluginInterface.h"

PluginManager::PluginManager()
{

}
      
void      
PluginManager::defaultLoad( QString baseDir )
{
    QDir workDir(baseDir);
/*  //=== find a file with last loaded plugins =====
    QStringList lastLoaded;
    QFile llFile( workDir.absolutePath()+"last_loaded.txt" )  ;
    if ( llFile.open(QIODevice::ReadOnly) ) 
    {
       QString tmps;
       QTextStream stream( &llFile ); // Set the stream to read from myFile
       tmps = stream.readLine(); 

       lastLoaded = tmps.split(";");
       llFile.close();
    }
*/
  //=== get current available plugins =====
    QStringList currAvailable = workDir.entryList(QDir::Files);
#if defined(Q_OS_WIN)    
    QRegExp trx("*.dll") ;
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
       readPluginFromFile( fullfn);       
    }//    foreach(QString pluginFileName, currAvailable)

    qDebug() << "  " << "names are " << names;
}

//=============================================================================

void
PluginManager::readPluginFromFile(QString fullFileName)
{
    qDebug() << "  " << "processing file " << fullFileName;

    QString errMess;
    PluginInterface* plugin = loadPluginInterface(fullFileName, errMess) ;

    QString plName = "Name undefined" ;
    QWidget* plWidget = 0;
    if (plugin)
    {
        plName = plugin->pluginName();
        qDebug() << "  " << "got pluginName:"  << plName;
/*               
	 //=== load widget (only  if this plugin was loaded last time)  
	   if ( lastLoaded.indexOf( pluginFileName) >= 0)
	   {
	       plWidget = plugin->pluginWidget() ;
               if (plWidget)
	       {
                   // all was good, 
                   qDebug() << "  " << "got widget..." ;
                   emit loadDone( plName, plWidget);
               }
               else
               {
                   errMess = "Plugin Object can't create widget"  ;
	       }
           }
*/
       delete plugin;
 
       names.append(plName);
//       int tpi = (int)plWidget;
       widgets.append(plWidget);
       fileNames.append(fullFileName);

       emit installComplete(plName);
    }
    else
    {
        emit installFailed( plName, errMess);
    }
}

//=============================================================================

PluginInterface* 
PluginManager::loadPluginInterface(QString fileName, QString& errorMessage)
{
    errorMessage = "Default Error Message" ;
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
	}        
    }
    else
    {
       errorMessage = "Istance wasn't created: " + plLoader->errorString() ;
    }


    delete plLoader; // plugin instance will not be deleted with this action

    return plugin;
}

/*
plName = plugin->pluginName();
//	       names.append(plddName);
	       qDebug() << "  " << "got pluginName:"  << plName;
               
	     //=== load widget (only  if this plugin was loaded last time)  
	       if ( lastLoaded.indexOf( pluginFileName) >= 0)
	       {
	           QWidget* pw = plugin->pluginWidget() ;
                   if (pw)
	           {
		       // all was good, 
		       qDebug() << "  " << "got widget..." ;
		       plState = PS_Loaded;
		       emit loadDone( plName, pw);
		   }
		   else
		   {
		      errMess = "Plugin Object can't create widget"  ;
		      plState = PS_Failed ;
		   }
	       }
	       else
	       {
		  plState = PS_Viewed ; //this is no error, all's normal
	       }

	   }
	   

//	       delete pluginObject;
       }
       else
       {
	  errMess = "Instance wasn't created: " + plLoader->errorString() ;
       }


}
*/
//=============================================================================

PluginManager:: ~PluginManager()
{
   // delete all widgets
   foreach(QWidget* wp, widgets)
   {
      if (wp)
	 delete wp;
   }   
}

//=============================================================================

QStringList 
PluginManager::availablePlugins()
{
    return names;
}
//=============================================================================
    
QStringList 
PluginManager::loadedPlugins()
{

}
     
bool 
PluginManager::isLoaded(QString pluginName)
{
}

//=============================================================================

void 
PluginManager::loadPlugin(QString pluginName)
{
   qDebug() << "" << "PluginManager::loadPlugin called for" << pluginName;

   int plIndex = names.indexOf( pluginName ) ;
   if (plIndex >=0 )
   {
      QWidget* plWidget = widgets.at(plIndex);
     //=== first, check if we loaded it before
      if ( plWidget )
      {
	 emit loadFailed( pluginName,
	                  "Error: plugin already loaded") ;
	 return ;
      }

    //=== next, load it's interface
      QString fn = fileNames.at(plIndex);
      QString errMess;
      PluginInterface* pliface = loadPluginInterface(fn, errMess);
      if (pliface)
      {
       //=== now, get a widget
	 plWidget = pliface->pluginWidget() ;
         if (plWidget)
         {
           // all was good,
             qDebug() << "  " << "got plg widget..." ;
             emit loadDone( pluginName, plWidget );
	     widgets[plIndex] = plWidget;
	     return;
         }
	 else
	 {
	    emit loadFailed(pluginName, 
		            "Error: instance can't create a widget");
	    return;
	 }
      }
      else
      {
          emit loadFailed( pluginName,
	      	           tr("Error: failed to load interface for ") 
			     + pluginName +"("+errMess+")");
	  return;
      }      
   }
   else
   {
       emit loadFailed( pluginName, 
	                tr("Error: know nothing about plugin ") + pluginName );

       qDebug() << "  " << "error: request to load " << pluginName
	                << " failed (unknown file)" ;
       return;
   }

}

//=============================================================================

void
PluginManager::unloadPlugin(QString pluginName)
{
    qDebug() << "  " << "PluginManager::unloadPlugin called for" << pluginName;

    int plIndex = names.indexOf( pluginName ) ;
    if (plIndex >=0 )
    {
        QWidget* plWidget =  widgets.at(plIndex);
	qDebug() << "in pm is " << (int)plWidget;
	if ( plWidget )
	{   //                    
	    widgets[plIndex] = 0;//(QWidget*)0;
	    qDebug() << "111" ;
	    delete plWidget;
	    qDebug() << "222" ;
	}
    }
}

//=============================================================================

