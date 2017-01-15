/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2011 Robert Fernie
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

#include <gui/statistics/TurtleRouterStatistics.h>
#include <gui/statistics/GlobalRouterStatistics.h>
#include <gui/statistics/BwCtrlWindow.h>
#include <gui/statistics/DhtWindow.h>

/****
 * #define SHOW_RTT_STATISTICS		1
 ****/
#define SHOW_RTT_STATISTICS		1

#ifdef SHOW_RTT_STATISTICS
	#include "gui/statistics/RttStatistics.h"
#endif

#define IMAGE_DHT           ":/images/dht32.png"
#define IMAGE_TURTLE        ":images/turtle.png"
#define IMAGE_BWGRAPH           ":/images/ksysguard.png"
#define IMAGE_GLOBALROUTER           ":/images/network32.png"
#define IMAGE_BANDWIDTH       ":images/office-chart-area-stacked.png"
#define IMAGE_RTT             ":images/office-chart-line.png"

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
   
    initStackedPage();
    connect(ui->stackPages, SIGNAL(currentChanged(int)), this, SLOT(setNewPage(int)));
    ui->stackPages->setCurrentIndex(0);

}

StatisticsWindow::~StatisticsWindow()
{
    delete ui;
    mInstance = NULL;
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
                   action = createPageAction(QIcon(IMAGE_BANDWIDTH), tr("Bandwidth"), grp));
                   
  ui->stackPages->add(trsdlg = new TurtleRouterStatistics(ui->stackPages),
                   action = createPageAction(QIcon(IMAGE_TURTLE), tr("Turtle Router"), grp));
                   
  ui->stackPages->add(grsdlg = new GlobalRouterStatistics(ui->stackPages),
                   action = createPageAction(QIcon(IMAGE_GLOBALROUTER), tr("Global Router"), grp)); 
                   
  ui->stackPages->add(rttdlg = new RttStatistics(ui->stackPages),
                      action = createPageAction(QIcon(IMAGE_RTT), tr("RTT Statistics"), grp));
                   
  ui->stackPages->add(dhtw = new DhtWindow(ui->stackPages),
                   action = createPageAction(QIcon(IMAGE_DHT), tr("DHT"), grp));

   /*std::cerr << "Looking for interfaces in existing plugins:" << std::endl;
	 for(int i = 0;i<rsPlugins->nbPlugins();++i)
	 {
		 QIcon icon ;

		 if(rsPlugins->plugin(i) != NULL && rsPlugins->plugin(i)->qt_page() != NULL)
		 {
			 if(rsPlugins->plugin(i)->qt_icon() != NULL)
				 icon = *rsPlugins->plugin(i)->qt_icon() ;
			 else
				 icon = QIcon(":images/extension_48.png") ;

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
    action->setFont(font);
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
