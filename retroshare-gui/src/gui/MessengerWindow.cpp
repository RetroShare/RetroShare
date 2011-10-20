/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2006, crypton
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

#include <QFile>
#include <QFileInfo>
#include <QWidgetAction>
#include <QTimer>

#include "common/vmessagebox.h"
#include "common/StatusDefs.h"

#include <retroshare/rspeers.h>
#include <retroshare/rsmsgs.h>
#include <retroshare/rsstatus.h>
#include <retroshare/rsnotify.h>

#include "rshare.h"
#include "MessengerWindow.h"
#include "RsAutoUpdatePage.h"

#ifndef MINIMAL_RSGUI
#include "MainWindow.h"
#include "ShareManager.h"
#include "notifyqt.h"
#include "connect/ConnectFriendWizard.h"
#endif // MINIMAL_RSGUI
#include "util/PixmapMerging.h"
#include "LogoBar.h"
#include "util/Widget.h"
#include "util/misc.h"
#include "settings/rsharesettings.h"

#include <iostream>
#include <sstream>
#include <algorithm>
#include <set>


/******
 * #define MSG_DEBUG 1
 *****/

MessengerWindow* MessengerWindow::_instance = NULL;
static std::set<std::string> *expandedPeers = NULL;

/*static*/ void MessengerWindow::showYourself ()
{
    if (_instance == NULL) {
        _instance = new MessengerWindow();
    }

    _instance->show();
    _instance->activateWindow();
}

MessengerWindow* MessengerWindow::getInstance()
{
    return _instance;
}

void MessengerWindow::releaseInstance()
{
    if (_instance) {
        delete _instance;
    }
    if (expandedPeers) {
        /* delete saved expanded peers */
        delete(expandedPeers);
        expandedPeers = NULL;
    }
}

/** Constructor */
MessengerWindow::MessengerWindow(QWidget* parent, Qt::WFlags flags)
    : 	RWindow("MessengerWindow", parent, flags)
{
    /* Invoke the Qt Designer generated object setup routine */
    ui.setupUi(this);

    setAttribute ( Qt::WA_DeleteOnClose, true );
#ifdef MINIMAL_RSGUI
    setAttribute (Qt::WA_QuitOnClose, true);
#endif // MINIMAL_RSGUI

    ui.avatar->setFrameType(AvatarWidget::STATUS_FRAME);
    ui.avatar->setOwnId();

#ifndef MINIMAL_RSGUI
    connect( ui.shareButton, SIGNAL(clicked()), SLOT(openShareManager()));
    connect( ui.addIMAccountButton, SIGNAL(clicked( bool ) ), this , SLOT( addFriend() ) );
#endif // MINIMAL_RSGUI
    connect(ui.actionHide_Offline_Friends, SIGNAL(toggled(bool)), ui.friendList, SLOT(setHideUnconnected(bool)));
    connect(ui.actionSort_by_State, SIGNAL(toggled(bool)), ui.friendList, SLOT(setSortByState(bool)));
    connect(ui.actionRoot_is_decorated, SIGNAL(toggled(bool)), ui.friendList, SLOT(setRootIsDecorated(bool)));
    connect(ui.clearButton, SIGNAL(clicked()), this, SLOT(clearFilter()));

    connect(ui.messagelineEdit, SIGNAL(textChanged(const QString &)), this, SLOT(savestatusmessage()));
    connect(ui.filterPatternLineEdit, SIGNAL(textChanged(const QString &)), this, SLOT(filterRegExpChanged()));

#ifndef MINIMAL_RSGUI
    connect(NotifyQt::getInstance(), SIGNAL(ownAvatarChanged()), this, SLOT(updateAvatar()));
    connect(NotifyQt::getInstance(), SIGNAL(ownStatusMessageChanged()), this, SLOT(loadmystatusmessage()));
    connect(NotifyQt::getInstance(), SIGNAL(peerStatusChanged(QString,int)), this, SLOT(updateOwnStatus(QString,int)));
#endif // MINIMAL_RSGUI

    if (expandedPeers != NULL) {
        for (std::set<std::string>::iterator peerIt = expandedPeers->begin(); peerIt != expandedPeers->end(); peerIt++) {
            ui.friendList->addPeerToExpand(*peerIt);
        }
        delete expandedPeers;
        expandedPeers = NULL;
    }

    //LogoBar
    _rsLogoBarmessenger = new LogoBar(ui.logoframe);
    Widget::createLayout(ui.logoframe)->addWidget(_rsLogoBarmessenger);

    ui.messagelineEdit->setMinimumWidth(20);

    displayMenu();

    // load settings
    RsAutoUpdatePage::lockAllEvents();
    processSettings(true);
    ui.friendList->setHideHeader(true);
    ui.friendList->setHideStatusColumn(true);
    ui.friendList->setHideGroups(true);
    ui.friendList->setBigName(true);
    RsAutoUpdatePage::unlockAllEvents();

    // add self nick
    RsPeerDetails pd;
    std::string ownId = rsPeers->getOwnId();
    if (rsPeers->getPeerDetails(ownId, pd)) {
        /* calculate only once */
        m_nickName = QString::fromUtf8(pd.name.c_str());
#ifdef MINIMAL_RSGUI
         ui.statusButton->setText(m_nickName);
#endif
    }

#ifndef MINIMAL_RSGUI
    /* Show nick and current state */
    StatusInfo statusInfo;
    rsStatus->getOwnStatus(statusInfo);
    updateOwnStatus(QString::fromStdString(ownId), statusInfo.status);

    MainWindow *pMainWindow = MainWindow::getInstance();
    if (pMainWindow) {
        QMenu *pStatusMenu = new QMenu();
        pMainWindow->initializeStatusObject(pStatusMenu, true);
        ui.statusButton->setMenu(pStatusMenu);
    }

    loadmystatusmessage();
#endif // MINIMAL_RSGUI

    ui.clearButton->hide();

    /* Hide platform specific features */
#ifdef Q_WS_WIN
#endif
}

MessengerWindow::~MessengerWindow ()
{
    // save settings
    processSettings(false);

#ifndef MINIMAL_RSGUI
    MainWindow *pMainWindow = MainWindow::getInstance();
    if (pMainWindow) {
        pMainWindow->removeStatusObject(ui.statusButton);
    }
#endif // MINIMAL_RSGUI

    _instance = NULL;
}

void MessengerWindow::processSettings(bool bLoad)
{
    Settings->beginGroup(_name);

    if (bLoad) {
        // load settings

        // state of messenger tree
        ui.friendList->restoreHeaderState(Settings->value("MessengerTree").toByteArray());

        // state of actionHide_Offline_Friends
        ui.actionHide_Offline_Friends->setChecked(Settings->value("hideOfflineFriends", false).toBool());

        // state of actionSort_by_State
        ui.actionSort_by_State->setChecked(Settings->value("sortByState", false).toBool());

        // state of actionRoot_is_decorated
        bool decorated = Settings->value("rootIsDecorated", true).toBool();
        ui.actionRoot_is_decorated->setChecked(decorated);
        ui.friendList->setRootIsDecorated(decorated);
    } else {
        // save settings

        // state of messenger tree
        Settings->setValue("MessengerTree", ui.friendList->saveHeaderState());

        // state of actionSort_by_State
        Settings->setValue("sortByState", ui.actionSort_by_State->isChecked());

        // state of actionHide_Offline_Friends
        Settings->setValue("hideOfflineFriends", ui.actionHide_Offline_Friends->isChecked());

        // state of actionRoot_is_decorated
        Settings->setValue("rootIsDecorated", ui.actionRoot_is_decorated->isChecked());
    }

    Settings->endGroup();
}

#ifndef MINIMAL_RSGUI
/** Add a Friend ShortCut */
void MessengerWindow::addFriend()
{
    ConnectFriendWizard connwiz (this);

    connwiz.exec ();
}
#endif

//============================================================================

void MessengerWindow::closeEvent (QCloseEvent * /*event*/)
{
    /* save the expanded peers */
    if (expandedPeers == NULL) {
        expandedPeers = new std::set<std::string>;
    } else {
        expandedPeers->clear();
    }

    ui.friendList->getExpandedPeers(*expandedPeers);
}

LogoBar & MessengerWindow::getLogoBar() const {
        return *_rsLogoBarmessenger;
}

#ifndef MINIMAL_RSGUI
/** Shows Share Manager */
void MessengerWindow::openShareManager()
{
	ShareManager::showYourself();
}

/** Loads own personal status message */
void MessengerWindow::loadmystatusmessage()
{ 
    ui.messagelineEdit->setEditText( QString::fromUtf8(rsMsgs->getCustomStateString().c_str()));
}

/** Save own status message */
void MessengerWindow::savestatusmessage()
{
    rsMsgs->setCustomStateString(ui.messagelineEdit->currentText().toUtf8().constData());
}

void MessengerWindow::updateOwnStatus(const QString &peer_id, int status)
{
    // add self nick + own status
    if (peer_id.toStdString() == rsPeers->getOwnId())
    {
        // my status has changed

        ui.statusButton->setText(m_nickName + " (" + StatusDefs::name(status) + ")");

        return;
    }
}

#endif // MINIMAL_RSGUI

void MessengerWindow::displayMenu()
{
    QMenu *lookmenu = new QMenu();
    lookmenu->addAction(ui.actionSort_Peers_Descending_Order);
    lookmenu->addAction(ui.actionSort_Peers_Ascending_Order);
    lookmenu->addAction(ui.actionSort_by_State);
    lookmenu->addAction(ui.actionHide_Offline_Friends);
    lookmenu->addAction(ui.actionRoot_is_decorated);

    ui.displaypushButton->setMenu(lookmenu);
}

/* clear Filter */
void MessengerWindow::clearFilter()
{
    ui.filterPatternLineEdit->clear();
    ui.filterPatternLineEdit->setFocus();
}

void MessengerWindow::filterRegExpChanged()
{

    QString text = ui.filterPatternLineEdit->text();

    if (text.isEmpty()) {
        ui.clearButton->hide();
    } else {
        ui.clearButton->show();
    }

    ui.friendList->filterItems(text);
}
