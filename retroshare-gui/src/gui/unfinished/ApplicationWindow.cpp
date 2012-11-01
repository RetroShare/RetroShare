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

#include <QCloseEvent>

#include <rshare.h>
#include "ApplicationWindow.h"

#include <retroshare/rsiface.h>

#include "gui/Identity/IdDialog.h"
#include "gui/PhotoShare/PhotoShare.h"
#include "gui/WikiPoos/WikiDialog.h"
#include "gui/Posted/PostedDialog.h"

// THESE HAVE TO BE CONVERTED TO VEG FORMAT
#if USE_VEG_SERVICE
#include "gui/TheWire/WireDialog.h"
#include "gui/ForumsV2Dialog.h"

#endif

//#include "GamesDialog.h"
//#include "CalDialog.h"
//#include "PhotoDialog.h"
//#include "StatisticDialog.h"

/* Images for toolbar icons */
#define IMAGE_RETROSHARE        ":/images/RetroShare16.png"
#define IMAGE_ABOUT             ":/images/informations_24x24.png"
#define IMAGE_STATISTIC         ":/images/ksysguard32.png"
#define IMAGE_GAMES             ":/images/kgames.png"
#define IMAGE_PHOTO             ":/images/lphoto.png"
#define IMAGE_BWGRAPH           ":/images/ksysguard.png"
#define IMAGE_CLOSE             ":/images/close_normal.png"
#define IMAGE_CALENDAR          ":/images/calendar.png"
#define IMAGE_LIBRARY           ":/images/library.png"
#define IMAGE_PLUGINS           ":/images/extension_32.png"
#define IMAGE_FORUMSV2            ":/images/konversation.png"

/** Constructor */
ApplicationWindow::ApplicationWindow(QWidget* parent, Qt::WFlags flags)
    : QMainWindow(parent, flags)
{
    /* Invoke the Qt Designer generated QObject setup routine */
    ui.setupUi(this);

    setWindowTitle(tr("RetroShare"));

    //Settings->loadWidgetInformation(this);

    // Setting icons
    this->setWindowIcon(QIcon(QString::fromUtf8(":/images/rstray3.png")));

    /* Create the config pages and actions */
    QActionGroup *grp = new QActionGroup(this);

    //StatisticDialog *statisticDialog = NULL;
    //ui.stackPages->add(statisticDialog = new StatisticDialog(ui.stackPages),
    //                   createPageAction(QIcon(IMAGE_STATISTIC), tr("Statistics"), grp));

    //GamesDialog *gamesDialog = NULL;
    //ui.stackPages->add(gamesDialog = new GamesDialog(ui.stackPages),
    //                   createPageAction(QIcon(IMAGE_GAMES), tr("Games Launcher"), grp));

    //CalDialog *calDialog = NULL;
    //ui.stackPages->add(calDialog = new CalDialog(ui.stackPages),
    //                  createPageAction(QIcon(IMAGE_CALENDAR), tr("Shared Calendars"), grp));

    IdDialog *idDialog = NULL;
    ui.stackPages->add(idDialog = new IdDialog(ui.stackPages),
                      createPageAction(QIcon(IMAGE_LIBRARY), tr("Identities"), grp));

    PhotoShare *photoShare = NULL;
    ui.stackPages->add(photoShare = new PhotoShare(ui.stackPages),
                     createPageAction(QIcon(IMAGE_PHOTO), tr("Photo Share"), grp));

    PostedDialog *postedDialog = NULL;
    ui.stackPages->add(postedDialog = new PostedDialog(ui.stackPages),
                      createPageAction(QIcon(IMAGE_LIBRARY), tr("Posted Links"), grp));

    WikiDialog *wikiDialog = NULL;
    ui.stackPages->add(wikiDialog = new WikiDialog(ui.stackPages),
                      createPageAction(QIcon(IMAGE_LIBRARY), tr("Wiki Pages"), grp));

// THESE HAVE TO BE CONVERTED TO VEG FORMAT
#if USE_VEG_SERVICE
    WireDialog *wireDialog = NULL;
    ui.stackPages->add(wireDialog = new WireDialog(ui.stackPages),
                      createPageAction(QIcon(IMAGE_BWGRAPH), tr("The Wire"), grp));

    ForumsV2Dialog *forumsV2Dialog = NULL;
    ui.stackPages->add(forumsV2Dialog = new ForumsV2Dialog(ui.stackPages),
                      createPageAction(QIcon(IMAGE_FORUMSV2), tr("ForumsV2"), grp));


#endif

   /* Create the toolbar */
   ui.toolBar->addActions(grp->actions());
   ui.toolBar->addSeparator();
   connect(grp, SIGNAL(triggered(QAction *)), ui.stackPages, SLOT(showPage(QAction *)));

   ui.stackPages->setCurrentIndex(0);
}

/** Creates a new action associated with a config page. */
QAction* ApplicationWindow::createPageAction(QIcon img, QString text, QActionGroup *group)
{
    QFont font;
    QAction *action = new QAction(img, text, group);
    font = action->font();
    font.setPointSize(9);
    action->setCheckable(true);
    action->setFont(font);
    return action;
}

/** Adds the given action to the toolbar and hooks its triggered() signal to
 * the specified slot (if given). */
void ApplicationWindow::addAction(QAction *action, const char *slot)
{
    QFont font = action->font();
    font.setPointSize(9);
    action->setFont(font);
    ui.toolBar->addAction(action);
    connect(action, SIGNAL(triggered()), this, slot);
}

/** Destructor. */
ApplicationWindow::~ApplicationWindow()
{
// is this allocated anywhere ??
//    delete exampleDialog;
}

void ApplicationWindow::closeEvent(QCloseEvent *e)
{
    //Settings->saveWidgetInformation(this);

    hide();
    e->ignore();
}
