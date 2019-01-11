/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2006 - 2009 RetroShare Team
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 *  Boston, MA  02110-1301, USA.
 ****************************************************************/

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
    
//    this->setTitle("UnseenP2P plugins");

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
