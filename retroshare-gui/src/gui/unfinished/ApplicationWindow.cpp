/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2006, 2007 crypton
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

#include <QtGui>
#include <QMessageBox>
#include <QSystemTrayIcon>
#include <QString>
#include <QtDebug>
#include <QIcon>
#include <QPixmap>

#include <rshare.h>
#include "ApplicationWindow.h"

#include "rsiface/rsiface.h"

#include "GamesDialog.h"
#include "PhotoDialog.h"
#include "BlogDialog.h"
#include "CalDialog.h"
#include "PeersFeed.h"
#include "TransferFeed.h"
#include "MsgFeed.h"
#include "StatisticDialog.h"


#define FONT        QFont(tr("Arial"), 8)

/* Images for toolbar icons */
#define IMAGE_NETWORK           ":/images/network32.png"
#define IMAGE_PEERS         	":/images/groupchat.png"
#define IMAGE_SEARCH    	":/images/filefind.png"
#define IMAGE_TRANSFERS      	":/images/ktorrent.png"
#define IMAGE_FILES   	        ":/images/folder_green.png"
#define IMAGE_FORUMS   	        ":/images/konversation.png"
#define IMAGE_CHANNELS       	":/images/channels.png"
#define IMAGE_PREFERENCES       ":/images/settings16.png"
#define IMAGE_CHAT          	":/images/chats_24x24.png"
#define IMAGE_RETROSHARE        ":/images/RetroShare16.png"
#define IMAGE_ABOUT             ":/images/informations_24x24.png"
#define IMAGE_STATISTIC         ":/images/ksysguard32.png"
#define IMAGE_GAMES             ":/images/kgames.png"
#define IMAGE_PHOTO             ":/images/lphoto.png"
#define IMAGE_BWGRAPH           ":/images/ksysguard.png"
#define IMAGE_RSM32             ":/images/rsmessenger32.png"
#define IMAGE_RSM16             ":/images/rsmessenger16.png"
#define IMAGE_CLOSE             ":/images/close_normal.png"
#define IMAGE_SMPLAYER          ":/images/smplayer_icon32.png"
#define IMAGE_CALENDAR          ":/images/calendar.png"
#define IMAGE_NEWSFEED          ":/images/konqsidebar_news24.png"
#define IMAGE_LINKS             ":/images/ktorrent.png"
#define IMAGE_MESSAGES          ":/images/evolution.png"
#define IMAGE_BLOGS             ":/images/kblogger.png"
#define IMAGE_LIBRARY           ":/images/library.png"
#define IMAGE_PLUGINS           ":/images/extension_32.png"



/* Keys for UI Preferences */
#define UI_PREF_PROMPT_ON_QUIT  "UIOptions/ConfirmOnQuit"
/* uncomment this for release version */
#define RS_RELEASE_VERSION    1

/** Constructor */
ApplicationWindow::ApplicationWindow(QWidget* parent, Qt::WFlags flags)
    : QMainWindow(parent, flags)
{
    /* Invoke the Qt Designer generated QObject setup routine */
    ui.setupUi(this);

    setWindowTitle(tr("RetroShare"));

    RshareSettings config;
    config.loadWidgetInformation(this);

    // Setting icons
    this->setWindowIcon(QIcon(QString::fromUtf8(":/images/rstray3.png")));
    loadStyleSheet("Default");

    /* Create the config pages and actions */
    QActionGroup *grp = new QActionGroup(this);

    StatisticDialog *statisticDialog = NULL;
    ui.stackPages->add(statisticDialog = new StatisticDialog(ui.stackPages),
                       createPageAction(QIcon(IMAGE_STATISTIC), tr("Statistics"), grp));

    PeersFeed *peersFeed = NULL;
    ui.stackPages->add(peersFeed = new PeersFeed(ui.stackPages),
                      createPageAction(QIcon(IMAGE_PEERS), tr("Peers"), grp));

    TransferFeed *transferFeed = NULL;
    ui.stackPages->add(transferFeed = new TransferFeed(ui.stackPages),
                      createPageAction(QIcon(IMAGE_LINKS), tr("Transfers"), grp));

    MsgFeed *msgFeed = NULL;
    ui.stackPages->add(msgFeed = new MsgFeed(ui.stackPages),
                      createPageAction(QIcon(IMAGE_MESSAGES), tr("Messages"), grp));

    BlogDialog *blogDialog = NULL;
    ui.stackPages->add(blogDialog = new BlogDialog(ui.stackPages),
                      createPageAction(QIcon(IMAGE_BLOGS), tr("Blog Feed"), grp));


    GamesDialog *gamesDialog = NULL;
    ui.stackPages->add(gamesDialog = new GamesDialog(ui.stackPages),
                       createPageAction(QIcon(IMAGE_GAMES), tr("Games Launcher"), grp));

    PhotoDialog *photoDialog = NULL;
    ui.stackPages->add(photoDialog = new PhotoDialog(ui.stackPages),
                      createPageAction(QIcon(IMAGE_PHOTO), tr("Photo View"), grp));


    CalDialog *calDialog = NULL;
    ui.stackPages->add(calDialog = new CalDialog(ui.stackPages),
                      createPageAction(QIcon(IMAGE_CALENDAR), tr("Shared Calendars"), grp));



   /* Create the toolbar */
   ui.toolBar->addActions(grp->actions());
   ui.toolBar->addSeparator();
   connect(grp, SIGNAL(triggered(QAction *)), ui.stackPages, SLOT(showPage(QAction *)));

    /* Create and bind the messenger button */
    //addAction(new QAction(QIcon(IMAGE_SMPLAYER), tr("SMPlayer"), ui.toolBar), SLOT(showsmplayer()));

}

/** Creates a new action associated with a config page. */
QAction* ApplicationWindow::createPageAction(QIcon img, QString text, QActionGroup *group)
{
    QAction *action = new QAction(img, text, group);
    action->setCheckable(true);
    action->setFont(FONT);
    return action;
}

/** Adds the given action to the toolbar and hooks its triggered() signal to
 * the specified slot (if given). */
void ApplicationWindow::addAction(QAction *action, const char *slot)
{
    action->setFont(FONT);
    ui.toolBar->addAction(action);
    connect(action, SIGNAL(triggered()), this, slot);
}

/** Overloads the default show so we can load settings */
void ApplicationWindow::show()
{

    if (!this->isVisible()) {
        QMainWindow::show();
    } else {
        QMainWindow::activateWindow();
        setWindowState(windowState() & ~Qt::WindowMinimized | Qt::WindowActive);
        QMainWindow::raise();
    }
}


/** Shows the config dialog with focus set to the given page. */
void ApplicationWindow::show(Page page)
{
    /* Show the dialog. */
    show();

    /* Set the focus to the specified page. */
    ui.stackPages->setCurrentIndex((int)page);
}


/** Destructor. */
ApplicationWindow::~ApplicationWindow()
{
// is this allocated anywhere ??
//    delete exampleDialog;
}

/** Create and bind actions to events. Setup for initial
 * tray menu configuration. */
void ApplicationWindow::createActions()
{
}

void ApplicationWindow::closeEvent(QCloseEvent *e)
{
    RshareSettings config;
    config.saveWidgetInformation(this);

    hide();
    e->ignore();
}


void ApplicationWindow::updateMenu()
{
    toggleVisibilityAction->setText(isVisible() ? tr("Hide") : tr("Show"));
}

void ApplicationWindow::toggleVisibility(QSystemTrayIcon::ActivationReason e)
{
    if(e == QSystemTrayIcon::Trigger || e == QSystemTrayIcon::DoubleClick){
        if(isHidden()){
            show();
            if(isMinimized()){
                if(isMaximized()){
                    showMaximized();
                }else{
                    showNormal();
                }
            }
            raise();
            activateWindow();
        }else{
            hide();
        }
    }
}

void ApplicationWindow::toggleVisibilitycontextmenu()
{
    if (isVisible())
        hide();
    else
        show();
}



void ApplicationWindow::loadStyleSheet(const QString &sheetName)
{
    QFile file(":/qss/" + sheetName.toLower() + ".qss");
    file.open(QFile::ReadOnly);
    QString styleSheet = QLatin1String(file.readAll());


    qApp->setStyleSheet(styleSheet);

}

/** Shows smplayer */
void ApplicationWindow::showsmplayer()
{
#if 0
    static SMPlayer * smplayer = 0;

    if (smplayer == 0) {
        smplayer = new SMPlayer(QString::null, this);
    }

    smplayer->gui()->show();
#endif
}

