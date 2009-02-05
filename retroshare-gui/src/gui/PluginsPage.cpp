#include <QGroupBox>
#include <QFrame>
#include <QVBoxLayout>
#include <QTabWidget>
#include <QLabel>
#include <QSizePolicy>
#include <QTextEdit>

#include <QtPlugin>
#include <QPluginLoader>
#include <QDir>
#include <QDebug>
#include <QStringList>
#include <QRegExp>

#include <QApplication> // for qApp->....
//#include <QUiLoader>
//#include <QtScript>
#include <QFile>
//#include <QIODevice>
#include <QMessageBox>
#include <QGroupBox>

#include "PluginsPage.h"
#include "PluginManagerWidget.h"
#include "PluginManager.h"
#include "rshare.h"
//#include "plugins/PluginInterface.h"

//============================================================================== 

PluginsPage::PluginsPage(QWidget *parent )
//            :QGroupBox(parent)
            :MainPage(parent)
{
  //=== create some gui elements =====
    pluginPageLayout = new QVBoxLayout(this);
    
//    pluginPanel = new QGroupBox(this) ;
// pluginPanel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding) ;
//    this->setTitle("RetroShare plugins");

    pluginTabs = new QTabWidget(this) ;
    pluginPageLayout->addWidget(pluginTabs);

    pmFrame = new QFrame;
    pmLay = new QVBoxLayout(pmFrame);

    pluginManagerWidget = new PluginManagerWidget(pmFrame);
    pmLay->addWidget( pluginManagerWidget );

    pmSpacer = new QSpacerItem(283, 20,
                            QSizePolicy::Expanding, QSizePolicy::Minimum);
    pmLay->addItem(pmSpacer);

    errlogConsole = new QTextEdit();
    pmLay->addWidget( errlogConsole );

    

    pluginTabs->addTab( pmFrame, "Manager" ) ;
    
//    pluginTabs->addTab( errlogConsole, "Error messages" );

  //=== try to load binary plugins =====
    pluginManager = new PluginManager();
    connect( pluginManager, SIGNAL( loadDone(QString, QWidget*) ),
	     this         , SLOT( loadDone(QString, QWidget*) ) );
    connect( pluginManager, SIGNAL(loadFailed(QString, QString)) ,
	     this         , SLOT( loadFailed(QString, QString ) ) );
    connect( pluginManagerWidget, SIGNAL( needToLoadFileWithPlugin(QString)),
	     pluginManager      , SLOT( readPluginFromFile(QString) ) );

    connect( pluginManager, SIGNAL( installComplete(QString) ),
	     this         , SLOT( installComplete(QString) ) );
    connect( pluginManager, SIGNAL(installFailed(QString, QString)) ,
	     this         , SLOT( installFailed(QString, QString ) ) );

    pluginManager->defaultLoad( Rshare::dataDirectory() + "/plugins" );
  // a variant for a toy app: qApp->applicationDirPath()+"///plugins" ) ;

    //QStringList lll = pluginManager->availablePlugins();
    //foreach( QString pn, lll)
    //{
    //   qDebug() << "  " << "----" << pn << "----" ;       
    //}

/*
    QDir pluginsDir(qApp->applicationDirPath()+"///plugins");
    if (pluginsDir.exists())
       loadBinaryPlugins( pluginsDir );
    else
    {
       qDebug() << "  " << "plugins will not be loaded: " << pluginsDir.absolutePath() ;
    }
*/
//    pluginPageLayout->addWidget(pluginPanel);
  /*  
    pluginPanelLayout = new QVBoxLayout(pluginPanel);
    
    pluginTabs = new QTabWidget(pluginPanel);
    pluginPanelLayout->addWidget(pluginTabs);
    
    QLabel* lbl1 = new QLabel();
    lbl1->setText("If you see only this tab, it's a bug :( If you see this tub and calculator, it's a bug too");
    pluginTabs->addTab(lbl1, "LLL");//Rshare::dataDirectory());
    //"L #1");
    
    //QLabel* lbl2 = new QLabel();
    //lbl2->setText("Label #2");
    //pluginTabs->addTab(lbl2, "L #2; for debugging purposes");

    QDir pluginsDir(Rshare::dataDirectory());

// this piece of code is magical and untested ;
// but on Windows it works
#if defined(Q_OS_WIN)
    if (pluginsDir.dirName().toLower() == "debug" || pluginsDir.dirName().toLower() == "release")
        pluginsDir.cdUp();
#elif defined(Q_OS_MAC)
    if (pluginsDir.dirName() == "MacOS") {
        pluginsDir.cdUp();
        pluginsDir.cdUp();
        pluginsDir.cdUp();
    }
#endif
// eof magick

    pluginsDir.cd("plugins");

    foreach (QString fileName, pluginsDir.entryList(QDir::Files))
    {
        qDebug() << "  "
                 << "processing file "
                 << pluginsDir.absoluteFilePath(fileName);

        QPluginLoader loader(pluginsDir.absoluteFilePath(fileName));
        QObject *pluginObject = loader.instance();
        if (pluginObject)
        {
            qDebug() << "  " << "loaded..." ;
            PluginInterface* plugin = qobject_cast<PluginInterface*> (pluginObject) ;

            if (plugin)
            {
                QString pn = plugin->pluginName();
                qDebug() << "  " << "got description:"  << pn;

                QWidget* pw = plugin->pluginWidget();
                qDebug() << "  " << "got widget..." ;

                pluginsDir.absoluteFilePath(fileName);
                pluginTabs->addTab(pw,pn);
            }
            else
            {
                qDebug() << "  " << "cast failed..." ;
            }
        }
    }

    // script plugins loading; warning, code is dirty; will be changed later
    engine = new QScriptEngine;

    pluginsDir.cd("../script_plugins");
    
    
 
    foreach (QString scriptDirName, pluginsDir.entryList(QDir::Dirs))
    {
        qDebug() << "  "  << "sdn is " << scriptDirName ;
        if ( (scriptDirName !=".") && (scriptDirName != "..") )
        {
            QDir spDir(pluginsDir) ;
            spDir.cd(scriptDirName);
            QStringList scfList = spDir.entryList(QDir::Files) ;
            qDebug() << "  "
                     << "to process files: "
                     << scfList ;// pluginsDir.absoluteFilePath(fileName);

            int ti;
            
            QRegExp rx_qs(".*js");
            
            ti = scfList.indexOf(rx_qs);
            QFile scriptFile( spDir.absoluteFilePath( scfList.at(ti) ) );
            scriptFile.open(QIODevice::ReadOnly);
            engine->evaluate(scriptFile.readAll());
            scriptFile.close();

            QUiLoader loader;
            QRegExp rx_ui(".*ui");
            ti = scfList.indexOf(rx_ui) ;
            QFile uiFile( spDir.absoluteFilePath( scfList.at(ti) ) );
            qDebug() << "ui file is " << scfList.at(ti) ;
            uiFile.open(QIODevice::ReadOnly);
            
            QWidget *ui = loader.load(&uiFile);
            uiFile.close();

            QScriptValue ctor = engine->evaluate("Plugin");
            QScriptValue scriptUi = engine->newQObject(ui, QScriptEngine::ScriptOwnership);
            QScriptValue calc = ctor.construct(QScriptValueList() << scriptUi);

            if (!ui)
                qDebug() << "ui is null :(" ;

            //ui->show();
            pluginTabs->addTab(ui,"Script plugin");            
        }
        //What has a head like a cat, feet like a cat, a tail like a cat, but isn't a cat?
    }
    */
}

//==============================================================================

PluginsPage::~PluginsPage()
{
    // remove all pages, exept first (i.e remove all pages with plugins) 
    for( int pi=1; pi<pluginTabs->count(); pi++)
    {
       pluginTabs->removeTab(pi); // widgets itself will be deleted 
                                //in ~PluginManager()
    }
}

//=============================================================================

void
PluginsPage::installComplete(QString pluginName)
{
    PluginFrame* pf = new PluginFrame( pluginManagerWidget, pluginName);
	  
    connect( pluginManager, SIGNAL( loadDone(QString, QWidget*) ),
             pf           , SLOT( successfulLoad(QString, QWidget*) ) );
 
    connect( pf           , SIGNAL( needToLoad(QString) ),
             pluginManager, SLOT( loadPlugin(QString) ) );

//       connect( pf           , SIGNAL( needToUnload(QString) ),
//	        pluginManager, SLOT( unloadPlugin(QString) ) );
       
    connect( pf           , SIGNAL( needToUnload(QString) ),
             this         , SLOT( unloadPlugin(QString) ) );

    pluginManagerWidget->addPluginWidget(pf);
}

//=============================================================================

void
PluginsPage::installFailed(QString pluginFileName, QString errorMessage)
{
   QString tmps = QString("failed to install plugin from %1  (%2)")
                         .arg(pluginFileName)
			 .arg(errorMessage);
   errlogConsole->append(tmps);
}

//=============================================================================

void
PluginsPage::loadDone(QString pluginName, QWidget* pluginWidget)
{
   pluginTabs->addTab( pluginWidget, pluginName );
}

//==============================================================================

void
PluginsPage::loadFailed(QString pluginName, QString errorMessage)
{
   QString tmps = QString("failed to load plugin %1  (%2)")
                         .arg(pluginName)
			 .arg(errorMessage);
   errlogConsole->append(tmps);
}

//==============================================================================

void
PluginsPage::unloadPlugin(QString pluginName)
{
   qDebug() << "  " << "PluginsPage::unloadPlugin called for " << pluginName ; 
   for (int tabi=0; tabi< pluginTabs->count(); tabi++)
   {
      if( pluginTabs->tabText(tabi) == pluginName)
      {
	 QWidget* tw= pluginTabs->widget(tabi);			  
	 pluginTabs->removeTab( tabi);//the plugin widget will not be deleted !!
                                    //plugin manager will delete it in
                                  //it's unloadPlugin 
//         qDebug() << "  " << "in pp " << (int) tw ;         
//	 delete tw; 

	 pluginManager->unloadPlugin( pluginName );
         return;
      }
   }
}

//==============================================================================

int
PluginsPage::loadBinaryPlugins(const QDir directory)
{/*
  //=== find a file with last loaded plugins =====
    QStringList lastLoaded;
    QFile llFile( directory.absolutePath()+"last_loaded.txt" )  ;
    if ( llFile.open(QIODevice::ReadOnly) ) 
    {
       QString tmps;
       QTextStream stream( &llFile ); // Set the stream to read from myFile
       tmps = stream.readLine(); 

       lastLoaded = tmps.split(";");
       llFile.close();
    }

  //=== get current available plugins =====
    QStringList currAvailable = directory.entryList(QDir::Files);
#if defined(Q_OS_WIN)    
    QRegExp trx("*.dll")
#else
    QRegExp trx("*.so");
#endif
    trx.setPatternSyntax(QRegExp::Wildcard );

    currAvailable.filter( trx );

    qDebug() << "  " << "can load this plugins: " << currAvailable ;

  //=== create widgets for all available; also load which were loaded before
    foreach(QString pluginFileName, currAvailable)
    {
       QString tmps( directory.absoluteFilePath( pluginFileName ) );
       pluginManagerWidget->addPluginWidget( tmps );

       
    }

*/
    

}

//==============================================================================


//=============================================================================

//==============================================================================
