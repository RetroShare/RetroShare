/*******************************************************************************
 * gui/statistics/StatisticsWindow.cpp                                         *
 *                                                                             *
 * Copyright (c) 2011 Robert Fernier  <retroshare.project@gmail.com>           *
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

#include "StatisticsWindow.h"
#include "ui_StatisticsWindow.h"
#include <QTimer>
#include <QDateTime>

#include <algorithm>
#include <iostream>
#include <iomanip>
#include <time.h>

#include "retroshare-gui/RsAutoUpdatePage.h"
#include "retroshare/rsconfig.h"
#include "retroshare/rspeers.h"
#include <retroshare/rsplugin.h>

#include <gui/settings/rsharesettings.h>
#include "gui/common/FilesDefs.h"

#include <gui/statistics/TurtleRouterStatistics.h>
#include <gui/statistics/GlobalRouterStatistics.h>
#include <gui/statistics/GxsIdStatistics.h>
#include <gui/statistics/GxsTransportStatistics.h>
#include <gui/statistics/BwCtrlWindow.h>
#include <gui/statistics/DhtWindow.h>

/****
 * #define SHOW_RTT_STATISTICS		1
 ****/
#define SHOW_RTT_STATISTICS		1

#ifdef SHOW_RTT_STATISTICS
	#include "gui/statistics/RttStatistics.h"
#endif

#define IMAGE_DHT           ":/icons/DHT128.png"
#define IMAGE_TURTLE        ":/icons/turtle128.png"
#define IMAGE_IDENTITIES    ":/icons/avatar_128.png"
#define IMAGE_BWGRAPH       ":/icons/bandwidth128.png"
#define IMAGE_GLOBALROUTER  ":/icons/GRouter128.png"
#define IMAGE_GXSTRANSPORT  ":/icons/transport128.png"
#define IMAGE_RTT           ":/icons/RTT128.png"

//#define IMAGE_BANDWIDTH     ":images/office-chart-area-stacked.png"

/********************************************** STATIC WINDOW *************************************/
StatisticsWindow * StatisticsWindow::mInstance = NULL;

void StatisticsWindow::showYourself()
{
    if (mInstance == NULL) {
        mInstance = new StatisticsWindow();
    }

    mInstance->show();
    mInstance->activateWindow();
}

StatisticsWindow* StatisticsWindow::getInstance()
{
    return mInstance;
}

void StatisticsWindow::releaseInstance()
{
    if (mInstance) {
        delete mInstance;
    }
}

/********************************************** STATIC WINDOW *************************************/



StatisticsWindow::StatisticsWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::StatisticsWindow)
{
    ui->setupUi(this);
   
	Settings->loadWidgetInformation(this);
	
    initStackedPage();
    connect(ui->stackPages, SIGNAL(currentChanged(int)), this, SLOT(setNewPage(int)));
    ui->stackPages->setCurrentIndex(0);
	int toolSize = Settings->getToolButtonSize();
	ui->toolBar->setToolButtonStyle(Settings->getToolButtonStyle());
	ui->toolBar->setIconSize(QSize(toolSize,toolSize));
}

StatisticsWindow::~StatisticsWindow()
{
    delete ui;
    mInstance = NULL;
}

void StatisticsWindow::closeEvent (QCloseEvent * /*event*/)
{
	Settings->saveWidgetInformation(this);
}

void StatisticsWindow::changeEvent(QEvent *e)
{
    QMainWindow::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

/** Initialyse Stacked Page*/
void StatisticsWindow::initStackedPage()
{

  QList<QPair<MainPage*, QAction*> > notify;

  /* Create the Main pages and actions */
  QActionGroup *grp = new QActionGroup(this);
  QAction *action;
  
  ui->stackPages->add(bwdlg = new BwCtrlWindow(ui->stackPages),
                   action = createPageAction(FilesDefs::getIconFromQtResourcePath(IMAGE_BWGRAPH), tr("Bandwidth"), grp));
                   
  ui->stackPages->add(trsdlg = new TurtleRouterStatistics(ui->stackPages),
                   action = createPageAction(FilesDefs::getIconFromQtResourcePath(IMAGE_TURTLE), tr("Turtle Router"), grp));
                   
  ui->stackPages->add(gxsiddlg = new GxsIdStatistics(ui->stackPages),
                   action = createPageAction(FilesDefs::getIconFromQtResourcePath(IMAGE_IDENTITIES), tr("Identities"), grp));

  ui->stackPages->add(grsdlg = new GlobalRouterStatistics(ui->stackPages),
                   action = createPageAction(FilesDefs::getIconFromQtResourcePath(IMAGE_GLOBALROUTER), tr("Global Router"), grp));
                   
  ui->stackPages->add(gxsdlg = new GxsTransportStatistics(ui->stackPages),
                   action = createPageAction(FilesDefs::getIconFromQtResourcePath(IMAGE_GXSTRANSPORT), tr("Gxs Transport"), grp));

  ui->stackPages->add(rttdlg = new RttStatistics(ui->stackPages),
                   action = createPageAction(FilesDefs::getIconFromQtResourcePath(IMAGE_RTT), tr("RTT Statistics"), grp));
                   
	bool showdht = true;
	RsPeerDetails detail;
	if (rsPeers->getPeerDetails(rsPeers->getOwnId(), detail))
	{
		if(detail.netMode == RS_NETMODE_HIDDEN)
			showdht = false;
	}
	if(showdht)
	{
	ui->stackPages->add(dhtw = new DhtWindow(ui->stackPages),
                   action = createPageAction(FilesDefs::getIconFromQtResourcePath(IMAGE_DHT), tr("DHT"), grp));
	}
	
   /*std::cerr << "Looking for interfaces in existing plugins:" << std::endl;
	 for(int i = 0;i<rsPlugins->nbPlugins();++i)
	 {
		 QIcon icon ;

		 if(rsPlugins->plugin(i) != NULL && rsPlugins->plugin(i)->qt_page() != NULL)
		 {
			 if(rsPlugins->plugin(i)->qt_icon() != NULL)
				 icon = *rsPlugins->plugin(i)->qt_icon() ;
			 else
                 icon = FilesDefs::getIconFromQtResourcePath(":images/extension_48.png") ;

			 std::cerr << "  Addign widget page for plugin " << rsPlugins->plugin(i)->getPluginName() << std::endl;
			 MainPage *pluginPage = rsPlugins->plugin(i)->qt_page();
			 QAction *pluginAction = createPageAction(icon, QString::fromUtf8(rsPlugins->plugin(i)->getPluginName().c_str()), grp);
			 ui->stackPages->add(pluginPage, pluginAction);
			 //notify.push_back(QPair<MainPage*, QAction*>(pluginPage, pluginAction));
		 }
		 else if(rsPlugins->plugin(i) == NULL)
			 std::cerr << "  No plugin object !" << std::endl;
		 else
			 std::cerr << "  No plugin page !" << std::endl;

	 } */                                                    

  /* Create the toolbar */
  ui->toolBar->addActions(grp->actions());

  connect(grp, SIGNAL(triggered(QAction *)), ui->stackPages, SLOT(showPage(QAction *)));

 }

/** Creates a new action associated with a main page. */
QAction *StatisticsWindow::createPageAction(const QIcon &icon, const QString &text, QActionGroup *group)
{
    QFont font;
    QAction *action = new QAction(icon, text, group);
    font = action->font();
    font.setPointSize(9);
    action->setCheckable(true);
//    action->setFont(font);
    return action;
}

/** Selection page. */
void StatisticsWindow::setNewPage(int page)
{
	MainPage *pagew = dynamic_cast<MainPage*>(ui->stackPages->widget(page)) ;

	if(pagew)
	{
		ui->stackPages->setCurrentIndex(page);
	}
}
