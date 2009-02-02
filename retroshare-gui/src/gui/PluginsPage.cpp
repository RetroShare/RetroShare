#include <QGroupBox>
#include <QFrame>
#include <QVBoxLayout>
#include <QTabWidget>
#include <QLabel>
#include <QSizePolicy>

#include <QtPlugin>
#include <QPluginLoader>
#include <QDir>
#include <QDebug>

#include <QUiLoader>
#include <QtScript>
#include <QFile>
#include <QMessageBox>

#include "PluginsPage.h"
#include "rshare.h"
#include "plugins/PluginInterface.h"

//============================================================================== 

PluginsPage::PluginsPage(QWidget *parent )
            :MainPage(parent)
{
    pluginPageLayout = new QVBoxLayout(this);
    
    pluginPanel = new QGroupBox(this) ;
    pluginPanel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding) ;
    pluginPanel->setTitle("RetroShare plugins");
    pluginPageLayout->addWidget(pluginPanel);
    
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

				if(ti > -1)
				{
					QFile scriptFile( spDir.absoluteFilePath( scfList.at(ti) ) );
					scriptFile.open(QIODevice::ReadOnly);
					engine->evaluate(scriptFile.readAll());
					scriptFile.close();
				}

            QUiLoader loader;
            QRegExp rx_ui(".*ui");
				QWidget *ui = NULL ;

				if(ti > -1)
				{
					ti = scfList.indexOf(rx_ui) ;
					QFile uiFile( spDir.absoluteFilePath( scfList.at(ti) ) );
					qDebug() << "ui file is " << scfList.at(ti) ;
					uiFile.open(QIODevice::ReadOnly);

					ui = loader.load(&uiFile);
					uiFile.close();
				}

            if (!ui)
                qDebug() << "ui is null :(" ;
				else
				{
					QScriptValue ctor = engine->evaluate("Plugin");
					QScriptValue scriptUi = engine->newQObject(ui, QScriptEngine::ScriptOwnership);
					QScriptValue calc = ctor.construct(QScriptValueList() << scriptUi);
				}


            //ui->show();
            pluginTabs->addTab(ui,"Script plugin");            
        }
        //What has a head like a cat, feet like a cat, a tail like a cat, but isn't a cat?
    }
    
}

//==============================================================================

PluginsPage::~PluginsPage()
{
    // nothing to do here at this moment
    delete engine;

}
