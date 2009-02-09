#include <QGroupBox>
#include <QFrame>
#include <QVBoxLayout>
#include <QTabWidget>
#include <QLabel>
#include <QSizePolicy>
#include <QTextEdit>

//#include <QtPlugin>
//#include <QPluginLoader>
//#include <QDir>
#include <QDebug>
#include <QStringList>
#include <QRegExp>

//#include <QApplication> // for qApp->....
//#include <QFile>

#include <QMessageBox>
#include <QGroupBox>

#include "PluginsPage.h"
#include "PluginManager.h"

//============================================================================== 

PluginsPage::PluginsPage(QWidget *parent )
//          :QGroupBox(parent) // this is for toy applications, do not remove
            :MainPage(parent)  // this for real retroshare app
{
  //=== 
    pluginManager = new PluginManager();
    connect( pluginManager, SIGNAL( newPluginRegistered(QString) ),
             this         , SLOT(   pluginRegistered(QString) ) );
    
  //=== create some gui elements =====
    pluginPageLayout = new QVBoxLayout(this);
    
//    this->setTitle("RetroShare plugins");

    pluginTabs = new QTabWidget(this) ;
    pluginPageLayout->addWidget(pluginTabs);

    pmFrame = new QFrame(this);
    pmLay = new QVBoxLayout(pmFrame);

    QWidget* tw = pluginManager->getViewWidget();

    pmLay->addWidget( tw );

    pmSpacer = new QSpacerItem(283, 20,
                               QSizePolicy::Expanding, QSizePolicy::Minimum);
    pmLay->addItem(pmSpacer);

    pluginTabs->addTab( pmFrame, "Manager" ) ;
    
    
    pluginManager->defaultLoad( ) ;
}

//==============================================================================

PluginsPage::~PluginsPage()
{
    delete pluginManager;
}

//=============================================================================

void
PluginsPage::pluginRegistered(QString pluginName)
{
    //
    QWidget* pw = pluginManager->pluginWidget( pluginName);

    pluginTabs->addTab( pw , pluginName );
}

//==============================================================================
