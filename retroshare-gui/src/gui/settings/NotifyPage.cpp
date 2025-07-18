/*******************************************************************************
 * gui/settings/NotifyPage.cpp                                                 *
 *                                                                             *
 * Copyright 2009, Retroshare Team <retroshare.project@gmail.com>              *
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

#include <rshare.h>
#include "NotifyPage.h"

#include <retroshare/rsnotify.h>
#include <retroshare/rsplugin.h>
#include "rsharesettings.h"

#include "gui/MainWindow.h"
#include "gui/common/UserNotify.h"
#include "gui/common/FeedNotify.h"
#include "gui/common/ToasterNotify.h"
#include "gui/notifyqt.h"
#include "gui/NewsFeed.h"
#include "util/misc.h"

/** Constructor */
NotifyPage::NotifyPage(QWidget * parent, Qt::WindowFlags flags)
  : ConfigPage(parent, flags)
{
	/* Invoke the Qt Designer generated object setup routine */
	ui.setupUi(this);

    ui.testFeedButton->hide();	// do not show in release

	connect(ui.testFeedButton, SIGNAL(clicked()), this, SLOT(testFeed()));
	connect(ui.testToasterButton, SIGNAL(clicked()), this, SLOT(testToaster()));
	connect(ui.pushButtonDisableAll,SIGNAL(toggled(bool)), NotifyQt::getInstance(), SLOT(SetDisableAll(bool)));
	connect(NotifyQt::getInstance(),SIGNAL(disableAllChanged(bool)), ui.pushButtonDisableAll, SLOT(setChecked(bool)));

	ui.notify_Blogs->hide();

	QFont font = ui.notify_Peers->font(); // use font from existing checkbox

	/* add feed and Toaster notify */
	int rowFeed = 0;
	int rowToaster = 0;
	int pluginCount = rsPlugins->nbPlugins();
	for (int i = 0; i < pluginCount; ++i)
	{
		RsPlugin *rsPlugin = rsPlugins->plugin(i);
		if (rsPlugin) {
			FeedNotify *feedNotify = rsPlugin->qt_feedNotify();
			if (feedNotify) {
				QString name;
				if (feedNotify->hasSetting(name)) {

					QCheckBox *enabledCheckBox = new QCheckBox(name, this);
					enabledCheckBox->setFont(font);
					ui.feedLayout->addWidget(enabledCheckBox, rowFeed++);

					mFeedNotifySettingList.push_back(FeedNotifySetting(feedNotify, enabledCheckBox));

					connect(enabledCheckBox,SIGNAL(toggled(bool)),this,SLOT(updateFeedNotifySettings()));
				}
			}

			ToasterNotify *toasterNotify = rsPlugin->qt_toasterNotify();
			if (toasterNotify) {
				QString name;
				if (toasterNotify->hasSetting(name)) {

					QCheckBox *enabledCheckBox = new QCheckBox(name, this);
					enabledCheckBox->setFont(font);
					ui.toasterLayout->addWidget(enabledCheckBox, rowToaster++);

					mToasterNotifySettingList.push_back(ToasterNotifySetting(toasterNotify, enabledCheckBox));

					connect(enabledCheckBox,SIGNAL(toggled(bool)),this,SLOT(updateToasterNotifySettings()));
				}

				QMap<QString, QString> map;
				if (toasterNotify->hasSettings(name, map)) {
					if (!map.empty()){
						QWidget* widget = new QWidget();
						QVBoxLayout* vbLayout = new QVBoxLayout(widget);
						QLabel *label = new QLabel(name, this);
						QFont fontBold = QFont(font);
						fontBold.setBold(true);
						label->setFont(fontBold);
						vbLayout->addWidget(label);
						for (QMap<QString, QString>::const_iterator it = map.begin(); it != map.end(); ++it){
							QCheckBox *enabledCheckBox = new QCheckBox(it.value(), this);
							enabledCheckBox->setAccessibleName(it.key());
							enabledCheckBox->setFont(font);
							vbLayout->addWidget(enabledCheckBox);
							mToasterNotifySettingList.push_back(ToasterNotifySetting(toasterNotify, enabledCheckBox));

							connect(enabledCheckBox,SIGNAL(toggled(bool)),this,SLOT(updateToasterNotifySettings())) ;
						}
						ui.toasterLayout->addWidget(widget, rowToaster++);
					}
				}
			}
		}

	}

    /* Add user notify */
    const QList<UserNotify*> &userNotifyList = MainWindow::getInstance()->getUserNotifyList() ;
    QList<UserNotify*>::const_iterator it;
    rowFeed = 0;
    for (it = userNotifyList.begin(); it != userNotifyList.end(); ++it) {
        UserNotify *userNotify = *it;

        QString name;
        if (!userNotify->hasSetting(&name, NULL)) {
            continue;
        }

        QCheckBox *enabledCheckBox = new QCheckBox(name, this);
        enabledCheckBox->setFont(font);
        ui.notifyLayout->addWidget(enabledCheckBox, rowFeed, 0, 0);
        connect(enabledCheckBox, SIGNAL(toggled(bool)), this, SLOT(notifyToggled()));

        QCheckBox *combinedCheckBox = new QCheckBox(tr("Combined"), this);
        combinedCheckBox->setFont(font);
        ui.notifyLayout->addWidget(combinedCheckBox, rowFeed, 1);

        QCheckBox *blinkCheckBox = new QCheckBox(tr("Blink"), this);
        blinkCheckBox->setFont(font);
        ui.notifyLayout->addWidget(blinkCheckBox, rowFeed++, 2);

        mUserNotifySettingList.push_back(UserNotifySetting(userNotify, enabledCheckBox, combinedCheckBox, blinkCheckBox));

        connect(enabledCheckBox,SIGNAL(toggled(bool)),this,SLOT(updateUserNotifySettings())) ;
        connect(blinkCheckBox,SIGNAL(toggled(bool)),this,SLOT(updateUserNotifySettings())) ;
        connect(combinedCheckBox,SIGNAL(toggled(bool)),this,SLOT(updateUserNotifySettings())) ;
    }

	connect(ui.popup_Connect,           SIGNAL(toggled(bool)), this, SLOT(updateNotifyFlags())) ;
	connect(ui.popup_NewMsg,            SIGNAL(toggled(bool)), this, SLOT(updateNotifyFlags())) ;
	connect(ui.popup_DownloadFinished,  SIGNAL(toggled(bool)), this, SLOT(updateNotifyFlags())) ;
	connect(ui.popup_PrivateChat,       SIGNAL(toggled(bool)), this, SLOT(updateNotifyFlags())) ;
	connect(ui.popup_GroupChat,         SIGNAL(toggled(bool)), this, SLOT(updateNotifyFlags())) ;
	connect(ui.popup_ChatLobby,         SIGNAL(toggled(bool)), this, SLOT(updateNotifyFlags())) ;
	connect(ui.popup_ConnectAttempt,    SIGNAL(toggled(bool)), this, SLOT(updateNotifyFlags())) ;

	connect(ui.message_ConnectAttempt,  SIGNAL(toggled(bool)), this, SLOT(updateMessageFlags())) ;

	connect(ui.notify_Peers,        SIGNAL(toggled(bool)), this, SLOT(updateNewsFeedFlags()));
	connect(ui.notify_Circles,      SIGNAL(toggled(bool)), this, SLOT(updateNewsFeedFlags()));
	connect(ui.notify_Channels,     SIGNAL(toggled(bool)), this, SLOT(updateNewsFeedFlags()));
	connect(ui.notify_Forums,       SIGNAL(toggled(bool)), this, SLOT(updateNewsFeedFlags()));
	connect(ui.notify_Posted,       SIGNAL(toggled(bool)), this, SLOT(updateNewsFeedFlags()));
	connect(ui.notify_Messages,     SIGNAL(toggled(bool)), this, SLOT(updateNewsFeedFlags()));
	connect(ui.notify_Chat,         SIGNAL(toggled(bool)), this, SLOT(updateNewsFeedFlags()));
	connect(ui.notify_Security,     SIGNAL(toggled(bool)), this, SLOT(updateNewsFeedFlags()));
	connect(ui.notify_SecurityIp,   SIGNAL(toggled(bool)), this, SLOT(updateNewsFeedFlags()));

#ifdef RS_USE_WIRE
    connect(ui.notify_Wire,         SIGNAL(toggled(bool)), this, SLOT(updateNewsFeedFlags()));
#endif

	connect(ui.systray_ChatLobby,   SIGNAL(toggled(bool)), this, SLOT(updateSystrayChatLobby()));
	connect(ui.systray_GroupChat,   SIGNAL(toggled(bool)), this, SLOT(updateSystrayGroupChat()));

	connect(ui.spinBoxToasterXMargin, SIGNAL(valueChanged(int)), this, SLOT(updateToasterMargin()));
	connect(ui.spinBoxToasterYMargin, SIGNAL(valueChanged(int)), this, SLOT(updateToasterMargin()));

	connect(ui.comboBoxToasterPosition,  SIGNAL(currentIndexChanged(int)),this, SLOT(updateToasterPosition())) ;
}

NotifyPage::~NotifyPage()
{
}

uint NotifyPage::getNewsFlags()
{
    uint newsFlags = 0;

    if (ui.notify_Peers->isChecked())
        newsFlags |= RS_FEED_TYPE_PEER;
    if (ui.notify_Circles->isChecked())
        newsFlags |= RS_FEED_TYPE_CIRCLE;
    if (ui.notify_Channels->isChecked())
        newsFlags |= RS_FEED_TYPE_CHANNEL;
    if (ui.notify_Forums->isChecked())
        newsFlags |= RS_FEED_TYPE_FORUM;
    if (ui.notify_Posted->isChecked())
        newsFlags |= RS_FEED_TYPE_POSTED;

#ifdef RS_USE_WIRE
    if (ui.notify_Wire->isChecked())
        newsFlags |= RS_FEED_TYPE_WIRE;
#endif

#if 0
    if (ui.notify_Blogs->isChecked())
        newsFlags |= RS_FEED_TYPE_BLOG;
#endif
    if (ui.notify_Messages->isChecked())
        newsFlags |= RS_FEED_TYPE_MSG;
    if (ui.notify_Chat->isChecked())
        newsFlags |= RS_FEED_TYPE_CHAT;
    if (ui.notify_Security->isChecked())
        newsFlags |= RS_FEED_TYPE_SECURITY;
    if (ui.notify_SecurityIp->isChecked())
        newsFlags |= RS_FEED_TYPE_SECURITY_IP;

    return newsFlags;
}

QString NotifyPage::helpText() const
{
			return tr("<h1><img width=\"24\" src=\":/icons/help_64.png\">&nbsp;&nbsp;Notify</h1> \
		  <p>Retroshare will notify you about what happens in your network.         \
		  Depending on your usage, you may want to enable or disable some of the    \
		  notifications. This page is designed for that!</p>                        \
		  ") ;

}

uint NotifyPage::getNotifyFlags()
{
    uint notifyFlags = 0;

    if (ui.popup_Connect->isChecked())
        notifyFlags |= RS_POPUP_CONNECT;
    if (ui.popup_NewMsg->isChecked())
        notifyFlags |= RS_POPUP_MSG;
    if (ui.popup_DownloadFinished->isChecked())
        notifyFlags |= RS_POPUP_DOWNLOAD;
    if (ui.popup_PrivateChat->isChecked())
        notifyFlags |= RS_POPUP_CHAT;
    if (ui.popup_GroupChat->isChecked())
        notifyFlags |= RS_POPUP_GROUPCHAT;
    if (ui.popup_ChatLobby->isChecked())
        notifyFlags |= RS_POPUP_CHATLOBBY;
    if (ui.popup_ConnectAttempt->isChecked())
        notifyFlags |= RS_POPUP_CONNECT_ATTEMPT;

    return notifyFlags;
}


void NotifyPage::updateFeedNotifySettings()
{
	/* save feed notify */
	QList<FeedNotifySetting>::iterator feedNotifyIt;
	for (feedNotifyIt = mFeedNotifySettingList.begin(); feedNotifyIt != mFeedNotifySettingList.end(); ++feedNotifyIt)
		feedNotifyIt->mFeedNotify->setNotifyEnabled(feedNotifyIt->mEnabledCheckBox->isChecked());
}

void NotifyPage::updateToasterNotifySettings()
{
    /* save toaster notify */
    QList<ToasterNotifySetting>::iterator toasterNotifyIt;
    for (toasterNotifyIt = mToasterNotifySettingList.begin(); toasterNotifyIt != mToasterNotifySettingList.end(); ++toasterNotifyIt) {
        if(toasterNotifyIt->mEnabledCheckBox->accessibleName().isEmpty())
            toasterNotifyIt->mToasterNotify->setNotifyEnabled(toasterNotifyIt->mEnabledCheckBox->isChecked()) ;
        else
            toasterNotifyIt->mToasterNotify->setNotifyEnabled(toasterNotifyIt->mEnabledCheckBox->accessibleName(), toasterNotifyIt->mEnabledCheckBox->isChecked()) ;
    }
}

void NotifyPage::updateUserNotifySettings()
{
	/* save user notify */
	QList<UserNotifySetting>::iterator notifyIt;
	for (notifyIt = mUserNotifySettingList.begin(); notifyIt != mUserNotifySettingList.end(); ++notifyIt)
		notifyIt->mUserNotify->setNotifyEnabled(notifyIt->mEnabledCheckBox->isChecked(), notifyIt->mCombinedCheckBox->isChecked(), notifyIt->mBlinkCheckBox->isChecked());

	MainWindow::installNotifyIcons();
}

void NotifyPage::updateMessageFlags() {  Settings->setMessageFlags( ui.message_ConnectAttempt->isChecked()? RS_MESSAGE_CONNECT_ATTEMPT : 0); }
void NotifyPage::updateNotifyFlags()  {	 Settings->setNotifyFlags(getNotifyFlags()); }
void NotifyPage::updateNewsFeedFlags(){  Settings->setNewsFeedFlags(getNewsFlags()); }

void NotifyPage::updateSystrayChatLobby() { Settings->setDisplayTrayChatLobby(ui.systray_ChatLobby->isChecked()); }
void NotifyPage::updateSystrayGroupChat() { Settings->setDisplayTrayGroupChat(ui.systray_GroupChat->isChecked()); MainWindow::installGroupChatNotifier(); }
void NotifyPage::updateToasterMargin()    { Settings->setToasterMargin(QPoint(ui.spinBoxToasterXMargin->value(), ui.spinBoxToasterYMargin->value())); }

void NotifyPage::updateToasterPosition()
{
    int index = ui.comboBoxToasterPosition->currentIndex();
    if (index != -1)
        Settings->setToasterPosition((RshareSettings::enumToasterPosition) ui.comboBoxToasterPosition->itemData(index).toInt());
}

/** Loads the settings for this page */
void NotifyPage::load()
{
	/* Extract from rsNotify the flags */
	uint notifyflags = Settings->getNotifyFlags() ;
	uint newsflags = Settings->getNewsFeedFlags() ;
	uint messageflags = Settings->getMessageFlags() ;

	whileBlocking(ui.popup_Connect)->setChecked(notifyflags & RS_POPUP_CONNECT);
	whileBlocking(ui.popup_NewMsg)->setChecked(notifyflags & RS_POPUP_MSG);
	whileBlocking(ui.popup_DownloadFinished)->setChecked(notifyflags & RS_POPUP_DOWNLOAD);
	whileBlocking(ui.popup_PrivateChat)->setChecked(notifyflags & RS_POPUP_CHAT);
#ifdef RS_DIRECT_CHAT
	whileBlocking(ui.popup_GroupChat)->setChecked(notifyflags & RS_POPUP_GROUPCHAT);
#endif // def RS_DIRECT_CHAT
	whileBlocking(ui.popup_ChatLobby)->setChecked(notifyflags & RS_POPUP_CHATLOBBY);
	whileBlocking(ui.popup_ConnectAttempt)->setChecked(notifyflags & RS_POPUP_CONNECT_ATTEMPT);

	whileBlocking(ui.notify_Peers)->setChecked(newsflags & RS_FEED_TYPE_PEER);
	whileBlocking(ui.notify_Circles)->setChecked(newsflags & RS_FEED_TYPE_CIRCLE);
	whileBlocking(ui.notify_Channels)->setChecked(newsflags & RS_FEED_TYPE_CHANNEL);
	whileBlocking(ui.notify_Forums)->setChecked(newsflags & RS_FEED_TYPE_FORUM);
	whileBlocking(ui.notify_Posted)->setChecked(newsflags & RS_FEED_TYPE_POSTED);

#ifdef RS_USE_WIRE
    whileBlocking(ui.notify_Wire)->setChecked(newsflags & RS_FEED_TYPE_WIRE);
#endif

#if 0
	whileBlocking(ui.notify_Blogs)->setChecked(newsflags & RS_FEED_TYPE_BLOG);
#endif
	whileBlocking(ui.notify_Chat)->setChecked(newsflags & RS_FEED_TYPE_CHAT);
	whileBlocking(ui.notify_Messages)->setChecked(newsflags & RS_FEED_TYPE_MSG);
	whileBlocking(ui.notify_Chat)->setChecked(newsflags & RS_FEED_TYPE_CHAT);
	whileBlocking(ui.notify_Security)->setChecked(newsflags & RS_FEED_TYPE_SECURITY);
	whileBlocking(ui.notify_SecurityIp)->setChecked(newsflags & RS_FEED_TYPE_SECURITY_IP);

	whileBlocking(ui.message_ConnectAttempt)->setChecked(messageflags & RS_MESSAGE_CONNECT_ATTEMPT);

	whileBlocking(ui.systray_GroupChat)->setChecked(Settings->getDisplayTrayGroupChat());
	whileBlocking(ui.systray_ChatLobby)->setChecked(Settings->getDisplayTrayChatLobby());

	whileBlocking(ui.pushButtonDisableAll)->setChecked(NotifyQt::isAllDisable());

    RshareSettings::enumToasterPosition toasterPosition = Settings->getToasterPosition();
    ui.comboBoxToasterPosition->clear();

    QMap<int, QString> toasterPositions;
    toasterPositions[RshareSettings::TOASTERPOS_TOPLEFT] = tr("Top Left");
    toasterPositions[RshareSettings::TOASTERPOS_TOPRIGHT] = tr("Top Right");
    toasterPositions[RshareSettings::TOASTERPOS_BOTTOMLEFT] = tr("Bottom Left");
    toasterPositions[RshareSettings::TOASTERPOS_BOTTOMRIGHT] = tr("Bottom Right");

    QMap<int, QString>::iterator it;
    int index = 0;
    for (it = toasterPositions.begin(); it != toasterPositions.end(); ++it, ++index) {
        whileBlocking(ui.comboBoxToasterPosition)->addItem(it.value(), it.key());

        if (it.key() == toasterPosition) {
            whileBlocking(ui.comboBoxToasterPosition)->setCurrentIndex(index);
        }
    }

	QPoint margin = Settings->getToasterMargin() ;
	whileBlocking(ui.spinBoxToasterXMargin)->setValue(margin.x());
	whileBlocking(ui.spinBoxToasterYMargin)->setValue(margin.y());

	/* Load feed notify */
	QList<FeedNotifySetting>::iterator feedNotifyIt ;
	for (feedNotifyIt = mFeedNotifySettingList.begin(); feedNotifyIt != mFeedNotifySettingList.end(); ++feedNotifyIt ) {
		whileBlocking(feedNotifyIt->mEnabledCheckBox)->setChecked(feedNotifyIt->mFeedNotify->notifyEnabled());
	}

	/* Load toaster notify */
	QList<ToasterNotifySetting>::iterator toasterNotifyIt ;
	for (toasterNotifyIt = mToasterNotifySettingList.begin(); toasterNotifyIt != mToasterNotifySettingList.end(); ++toasterNotifyIt ) {
		if( toasterNotifyIt->mEnabledCheckBox->accessibleName().isEmpty() ) {
			whileBlocking(toasterNotifyIt->mEnabledCheckBox)->setChecked(toasterNotifyIt->mToasterNotify->notifyEnabled()) ;
		} else {
			whileBlocking(toasterNotifyIt->mEnabledCheckBox)->setChecked(toasterNotifyIt->mToasterNotify->notifyEnabled(toasterNotifyIt->mEnabledCheckBox->accessibleName())) ;
		}
	}

	/* Load user notify */
	QList<UserNotifySetting>::iterator userNotifyIt ;
	for (userNotifyIt = mUserNotifySettingList.begin(); userNotifyIt != mUserNotifySettingList.end(); ++userNotifyIt ) {
		whileBlocking(userNotifyIt->mEnabledCheckBox)->setChecked(userNotifyIt->mUserNotify->notifyEnabled());
		whileBlocking(userNotifyIt->mCombinedCheckBox)->setChecked(userNotifyIt->mUserNotify->notifyCombined());
		whileBlocking(userNotifyIt->mBlinkCheckBox)->setChecked(userNotifyIt->mUserNotify->notifyBlink());
	}

	notifyToggled() ;

}

void NotifyPage::notifyToggled()
{
    QList<UserNotifySetting>::iterator notifyIt;
    for (notifyIt = mUserNotifySettingList.begin(); notifyIt != mUserNotifySettingList.end(); ++notifyIt) {
        if (notifyIt->mEnabledCheckBox->isChecked()) {
            notifyIt->mCombinedCheckBox->setEnabled(true);
            notifyIt->mBlinkCheckBox->setEnabled(true);
        } else {
            notifyIt->mCombinedCheckBox->setChecked(false);
            notifyIt->mCombinedCheckBox->setEnabled(false);

            notifyIt->mBlinkCheckBox->setChecked(false);
            notifyIt->mBlinkCheckBox->setEnabled(false);
        }
    }
}

void NotifyPage::testFeed()
{
    NewsFeed::testFeeds(getNewsFlags());

    /* notify of plugins */
    QList<FeedNotifySetting>::iterator feedNotifyIt;
    for (feedNotifyIt = mFeedNotifySettingList.begin(); feedNotifyIt != mFeedNotifySettingList.end(); ++feedNotifyIt) {
        if (feedNotifyIt->mEnabledCheckBox->isChecked()) {
            NewsFeed::testFeed(feedNotifyIt->mFeedNotify);
        }
    }
}

void NotifyPage::testToaster()
{
    RshareSettings::enumToasterPosition pos = (RshareSettings::enumToasterPosition) ui.comboBoxToasterPosition->itemData(ui.comboBoxToasterPosition->currentIndex()).toInt();
    QPoint margin = QPoint(ui.spinBoxToasterXMargin->value(), ui.spinBoxToasterYMargin->value());
    NotifyQt::getInstance()->testToasters(getNotifyFlags(), pos, margin);

    /* notify of plugins */
    QList<ToasterNotifySetting>::iterator toasterNotifyIt;
    for (toasterNotifyIt = mToasterNotifySettingList.begin(); toasterNotifyIt != mToasterNotifySettingList.end(); ++toasterNotifyIt) {
        if (toasterNotifyIt->mEnabledCheckBox->isChecked()){
            if (toasterNotifyIt->mEnabledCheckBox->accessibleName().isEmpty()){
                NotifyQt::getInstance()->testToaster(toasterNotifyIt->mToasterNotify, pos, margin) ;
            } else {
                NotifyQt::getInstance()->testToaster(toasterNotifyIt->mEnabledCheckBox->accessibleName(), toasterNotifyIt->mToasterNotify, pos, margin) ;
            }
        }
    }
}
