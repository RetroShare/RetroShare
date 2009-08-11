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

#include "PluginManagerWidget.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QWidget>
#include <QTextEdit>

#include <QFileDialog>

#include <QDebug>
#include <QObject>

#include "PluginManagerWidget.h"

//=============================================================================
//=============================================================================
//=============================================================================
PluginFrame::PluginFrame( QString pluginName, QWidget * parent)
                    :QFrame(parent)
{
   plgName = pluginName;
   
   // labels
   labelsLay = new QVBoxLayout() ;
   nameLabel = new QLabel();
   nameLabel->setText(pluginName);
   nameLabel->setAlignment(Qt::AlignHCenter);
   labelsLay->addWidget(nameLabel);
   descrLabel =  new QLabel();
   descrLabel->setText("# # # # # # # # # #");
//	 "plugin description  will appear here someday");
   descrLabel->setWordWrap(true);
   labelsLay->addWidget(descrLabel);

   // buttons
   buttonsLay = new QVBoxLayout() ;

   removeBtn = new QPushButton() ;
   removeBtn->setText( tr("Remove") ) ;
   connect( removeBtn, SIGNAL( clicked() ) ,
	        this     , SLOT  ( removeButtonClicked() ) )    ;
   buttonsLay->addWidget( removeBtn ) ;

   //all together
   mainLay = new QHBoxLayout(this);
   mainLay->addLayout(labelsLay);
   mainLay->addLayout(buttonsLay);

   this->setFrameStyle(QFrame::Box | QFrame::Raised);
}

//=============================================================================

PluginFrame::~PluginFrame()
{
   //nothing to do here at this moment
}

//=============================================================================

void
PluginFrame::removeButtonClicked()
{
    emit needToRemove( plgName );
}

//=============================================================================

QString
PluginFrame::getPluginName()
{
    return plgName ;
}

//=============================================================================
//=============================================================================
//=============================================================================

PluginManagerWidget::PluginManagerWidget(QWidget * parent)
                    :QFrame(parent)
{  
    qDebug() << "  " << "PluginManagerWidget::PluginManagerWidget here"; 

    mainLayout = new QVBoxLayout(this);

  //===
    installPluginLayout = new QHBoxLayout();
    installPluginButton = new QPushButton();
    
    installPluginButton->setText(tr("Install New Plugin..."));
    connect( installPluginButton, SIGNAL( clicked() ),
             this               , SLOT(   installPluginButtonClicked() ) );
    installPluginLayout->addWidget(installPluginButton);
    installPluginSpacer = new QSpacerItem(283, 20, 
                                  QSizePolicy::Expanding, QSizePolicy::Minimum);
    installPluginLayout->addItem(installPluginSpacer);

    mainLayout->addLayout( installPluginLayout );
    
  //===
    pluginFramesContainer = new QFrame();
    pluginFramesLayout = new QVBoxLayout(pluginFramesContainer);
    
    mainLayout->addWidget(pluginFramesContainer);
    
  //===
    errorsConsole = new QTextEdit();
    
    mainLayout->addWidget( errorsConsole );
   
    qDebug() << "  " << "PluginManagerWidget::PluginManagerWidget done"; 
}

//=============================================================================

PluginManagerWidget::~PluginManagerWidget()
{
    //nothing to do here
}

//=============================================================================

void
PluginManagerWidget::registerNewPlugin(QString pluginName)
{
    qDebug() << "  " << "PluginManagerWidget::registerNewPlugin "<< pluginName; 

    PluginFrame* pf = new PluginFrame(pluginName, pluginFramesContainer) ;
    
    connect( pf  , SIGNAL( needToRemove(QString)),
             this, SIGNAL( removeRequested(QString) ) );
    
    pluginFramesLayout->addWidget(pf);
}

//=============================================================================

void
PluginManagerWidget::installPluginButtonClicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, 
	                       tr("Open Plugin to install"),
	                       "./",
			       tr("Plugins (*.so *.dll)"));
    if (!fileName.isNull())
    {
       emit installPluginRequested(fileName);
    }
}

//=============================================================================

void
PluginManagerWidget::removePluginFrame(QString pluginName)
{
    foreach(QObject* ob, pluginFramesContainer->children())
    {
        PluginFrame* pf = qobject_cast<PluginFrame*> (ob);
        if (pf)
        {
            if (pf->getPluginName() == pluginName )
	        {
                pf->setParent(0);
	            delete pf;
		        return ;
	        }
	    }
    }

    // normally unreachable place
    QString em = QString("Widget for plugin %1 not found on plugins frame")
                    .arg( pluginName ) ;
    acceptErrorMessage( em );    
}

//=============================================================================

void 
PluginManagerWidget::acceptErrorMessage(QString errorMessage)
{
    errorsConsole->append( errorMessage );
}

//=============================================================================

