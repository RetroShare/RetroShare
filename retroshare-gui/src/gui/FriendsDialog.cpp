/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2006 - 2011 RetroShare Team
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

#include <time.h>

#include <QDropEvent>
#include <QMenu>
#include <QTimer>
#include <QMessageBox>

#include <retroshare/rspeers.h>
#include <retroshare/rshistory.h>
#include "chat/ChatUserNotify.h"
#include "connect/ConnectFriendWizard.h"
#include "groups/CreateGroup.h"
#include "MainWindow.h"
#include "NewsFeed.h"
#include "notifyqt.h"
#include "profile/ProfileWidget.h"
#include "profile/StatusMessage.h"
#include "RetroShareLink.h"
#include "settings/rsharesettings.h"
#include "util/misc.h"
#include "util/DateTime.h"
#include "FriendsDialog.h"
#include "NetworkView.h"
#include "NetworkDialog.h"
#include "gui/Identity/IdDialog.h"
#ifdef RS_USE_CIRCLES
#include "gui/Circles/CirclesDialog.h"
#endif
/* Images for Newsfeed icons */
//#define IMAGE_NEWSFEED           ""
//#define IMAGE_NEWSFEED_NEW       ":/images/message-state-new.png"
#define IMAGE_NETWORK2          ":/icons/png/netgraph.png"
#define IMAGE_PEERS         	":/icons/png/keyring.png"
#define IMAGE_IDENTITY          ":/images/identity/identities_32.png"
//#define IMAGE_CIRCLES           ":/icons/png/circles.png"

/******
 * #define FRIENDS_DEBUG 1
 *****/

static FriendsDialog *instance = NULL;

/** Constructor */
FriendsDialog::FriendsDialog(QWidget *parent)
            : RsAutoUpdatePage(1500,parent)
{
    /* Invoke the Qt Designer generated object setup routine */
    ui.setupUi(this);

	if (!instance) instance = this;

    connect( ui.mypersonalstatusLabel, SIGNAL(clicked()), SLOT(statusmessage()));
    connect( ui.actionSet_your_Avatar, SIGNAL(triggered()), this, SLOT(getAvatar()));
    connect( ui.actionSet_your_Personal_Message, SIGNAL(triggered()), this, SLOT(statusmessage()));

    ui.avatar->setOwnId();
    ui.avatar->setFrameType(AvatarWidget::STATUS_FRAME);

    ui.tabWidget->setTabPosition(QTabWidget::North);
    ui.tabWidget->addTab(networkView = new NetworkView(),QIcon(IMAGE_NETWORK2), tr("Network graph"));
    ui.tabWidget->addTab(networkDialog = new NetworkDialog(),QIcon(IMAGE_PEERS), tr("Keyring"));

    ui.tabWidget->hideCloseButton(0);
    ui.tabWidget->hideCloseButton(1);
    ui.tabWidget->hideCloseButton(2);
    ui.tabWidget->hideCloseButton(3);
    ui.tabWidget->hideCloseButton(4);

    /* Set initial size the splitter */
    ui.splitter->setStretchFactor(0, 0);
    ui.splitter->setStretchFactor(1, 1);
    /*remove
QList<int> sizes;
    sizes << height() << 100; // Qt calculates the right sizes
    ui.splitter_2->setSizes(sizes);*/

    loadmypersonalstatus();

    ui.mypersonalstatusLabel->setMinimumWidth(25);

    // load settings
    RsAutoUpdatePage::lockAllEvents();
    ui.friendList->setColumnVisible(FriendList::COLUMN_LAST_CONTACT, false);
    ui.friendList->setColumnVisible(FriendList::COLUMN_IP, false);
    ui.friendList->setColumnVisible(FriendList::COLUMN_ID, false);
    ui.friendList->setShowGroups(true);
    processSettings(true);
    RsAutoUpdatePage::unlockAllEvents();


    // add self nick and Avatar to Friends.
    RsPeerDetails pd ;
    if (rsPeers->getPeerDetails(rsPeers->getOwnId(),pd)) {
        ui.nicknameLabel->setText(QString::fromUtf8(pd.name.c_str()) + " (" + QString::fromUtf8(pd.location.c_str())+")");
    }

 QString hlp_str = tr(
  " <h1><img width=\"32\" src=\":/icons/help_64.png\">&nbsp;&nbsp;Network</h1>                                   \
    <p>The Network tab shows your friend Retroshare nodes: the neighbor Retroshare nodes that are connected to you. \
    </p>                                                   \
    <p>You can group nodes together to allow a finer level of information access, for instance to only allow      \
    some nodes to see some of your files.</p> \
    <p>On the right, you will find 3 useful tabs:                                                                   \
    <ul>                                                                                                          \
      <li>Broadcast sends messages to all connected nodes at once</li>                             \
      <li>Local network graph shows the network around you, based on discovery information</li>                 \
      <li>Keyring contains node keys you collected, mostly forwarded to you by your friend nodes</li>                              \
    </ul> </p>                                                                                                      \
  ") ;

	 registerHelpButton(ui.helpButton, hlp_str,"FriendsDialog") ;
}

FriendsDialog::~FriendsDialog ()
{
    // save settings
    processSettings(false);

    if (this == instance) {
        instance = NULL;
    }
}

void FriendsDialog::activatePage(FriendsDialog::Page page)
{
	switch(page)
	{
		case FriendsDialog::IdTab: ui.tabWidget->setCurrentWidget(idDialog) ;
											  break ;
#ifdef RS_USE_CIRCLES
		case FriendsDialog::CirclesTab: ui.tabWidget->setCurrentWidget(circlesDialog) ;
											  break ;
#endif
		case FriendsDialog::NetworkTab: ui.tabWidget->setCurrentWidget(networkDialog) ;
											  break ;
		case FriendsDialog::BroadcastTab: ui.tabWidget->setCurrentWidget(networkDialog) ;
											  break ;
		case FriendsDialog::NetworkViewTab: ui.tabWidget->setCurrentWidget(networkView) ;
											  break ;
	}
}

UserNotify *FriendsDialog::getUserNotify(QObject *parent)
{
    return new ChatUserNotify(parent);
}

void FriendsDialog::processSettings(bool bLoad)
{
    Settings->beginGroup(QString("FriendsDialog"));

    if (bLoad) {
        // load settings

        // state of splitter
        ui.splitter->restoreState(Settings->value("Splitter").toByteArray());
    } else {
        // save settings

        // state of splitter
        Settings->setValue("Splitter", ui.splitter->saveState());
    }

    ui.friendList->processSettings(bLoad);

    Settings->endGroup();
}

void FriendsDialog::showEvent(QShowEvent *event)
{
    RsAutoUpdatePage::showEvent(event);
}

void FriendsDialog::updateDisplay()
{
}

void FriendsDialog::addFriend()
{
    std::string groupId = ui.friendList->getSelectedGroupId();

    ConnectFriendWizard connwiz (this);

    if (groupId.empty() == false) {
        connwiz.setGroup(groupId);
    }

    connwiz.exec ();
}

void FriendsDialog::getAvatar()
{
	QByteArray ba;
	if (misc::getOpenAvatarPicture(this, ba))
	{
#ifdef FRIENDS_DEBUG
		std::cerr << "Avatar image size = " << ba.size() << std::endl ;
#endif

		rsMsgs->setOwnAvatarData((unsigned char *)(ba.data()), ba.size()) ;	// last char 0 included.
	}
}

/** Loads own personal status */
void FriendsDialog::loadmypersonalstatus()
{
	QString statustring =  QString::fromUtf8(rsMsgs->getCustomStateString().c_str());

	if (statustring.isEmpty())
	{
		ui.mypersonalstatusLabel->setText(tr("Set your status message here."));
	}
	else
	{
		ui.mypersonalstatusLabel->setText(statustring);
	}
}

void FriendsDialog::statusmessage()
{
    StatusMessage statusmsgdialog (this);
    statusmsgdialog.exec();
}

