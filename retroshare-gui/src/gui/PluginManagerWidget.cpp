#include "PluginManagerWidget.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QWidget>

#include <QFileDialog>

#include <QDebug>

//=============================================================================
//=============================================================================
//=============================================================================

enum { 
   LBS_Load , // load button state : load (it will send signal 'needToLoad')
   LBS_Unload // load button state: unload
};

PluginFrame::PluginFrame(QWidget * parent, QString pluginName)
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
   descrLabel->setText("plugin description  will appear here someday");
   descrLabel->setWordWrap(true);
   labelsLay->addWidget(descrLabel);

   // buttons
   buttonsLay = new QVBoxLayout() ;
   loadBtn = new QPushButton;
   loadBtn->setText( tr("Load") );
   loadBtnState = LBS_Load;
   connect( loadBtn, SIGNAL( clicked() ) ,
	    this   , SLOT  ( loadButtonClicked() ) );
   buttonsLay->addWidget( loadBtn );
   removeBtn = new QPushButton() ;
   removeBtn->setText( tr("Remove") ) ;
   buttonsLay->addWidget( removeBtn ) ;

   //all together
   mainLay = new QHBoxLayout(this);
   mainLay->addLayout(labelsLay);
   mainLay->addLayout(buttonsLay);

   this->setFrameStyle(QFrame::Box | QFrame::Raised);


   //hlay->addWidget(lbl);

}

//=============================================================================

PluginFrame::~PluginFrame()
{
   //nothing to do here at this moment
}

//=============================================================================

void
PluginFrame::loadButtonClicked()
{
    if (loadBtnState == LBS_Load)
    {
       emit needToLoad(plgName);
       return;
    }

    if ( loadBtnState == LBS_Unload)
    {
	emit needToUnload(plgName);
	loadBtn->setText( tr("Load") );
	loadBtnState = LBS_Load ;
    }
}

//=============================================================================

void
PluginFrame::removeButtonClicked()
{
   emit needToRemove( plgName );
}

//=============================================================================

void
PluginFrame::successfulLoad(QString pluginName, QWidget* wd)
{
   qDebug() << "  " << "PluginFrame::successfulLoad for " << pluginName 
            << " -- " << plgName  ;
   if (pluginName == plgName )
   {
      qDebug() << "so...";
      loadBtn->setText( tr("Unload") );  
      loadBtnState = LBS_Unload ;
   }
}

//=============================================================================
//=============================================================================
//=============================================================================

PluginManagerWidget::PluginManagerWidget(QWidget * parent)
                    :QFrame(parent)
{   
    vlay = new QVBoxLayout(this);

    instPlgLay = new QHBoxLayout();
    instPlgButton = new QPushButton();
    
    instPlgButton->setText("Install New Plugin...");
    connect( instPlgButton, SIGNAL( clicked() ),
	    this          , SLOT(   instPlgButtonClicked() ) );
    instPlgLay->addWidget(instPlgButton);
    instPlgSpacer = new QSpacerItem(283, 20, 
                          QSizePolicy::Expanding, QSizePolicy::Minimum);
    instPlgLay->addItem(instPlgSpacer);

    vlay->addLayout( instPlgLay );
}

//=============================================================================

PluginManagerWidget::~PluginManagerWidget()
{
}

//=============================================================================

void
PluginManagerWidget::addPluginWidget(PluginFrame* pf)
{
    vlay->addWidget(pf);
}

//=============================================================================

void
PluginManagerWidget::instPlgButtonClicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, 
	                       tr("Open Plugin to install"),
	                       "./",
			       tr("Plugins (*.so *.dll)"));
    if (!fileName.isNull())
    {
       emit needToLoadFileWithPlugin(fileName);
    }
}

