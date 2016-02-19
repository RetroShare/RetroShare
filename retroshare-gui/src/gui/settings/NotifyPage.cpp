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

/** Constructor */
NotifyPage::NotifyPage(QWidget * parent, Qt::WindowFlags flags)
  : ConfigPage(parent, flags)
{
  /* Invoke the Qt Designer generated object setup routine */
  ui.setupUi(this);

  connect(ui.testFeedButton, SIGNAL(clicked()), this, SLOT(testFeed()));
  connect(ui.testToasterButton, SIGNAL(clicked()), this, SLOT(testToaster()));
  connect(ui.pushButtonDisableAll,SIGNAL(toggled(bool)), NotifyQt::getInstance(), SLOT(SetDisableAll(bool)));
  connect(NotifyQt::getInstance(),SIGNAL(disableAllChanged(bool)), ui.pushButtonDisableAll, SLOT(setChecked(bool)));
    connect(ui.chatLobbies_CountFollowingText,SIGNAL(toggled(bool)),ui.chatLobbies_TextToNotify,SLOT(setEnabled(bool))) ;

  ui.notify_Blogs->hide();

  QFont font = ui.notify_Peers->font(); // use font from existing checkbox

  /* add feed and Toaster notify */
  int rowFeed = 0;
  int rowToaster = 0;
  int pluginCount = rsPlugins->nbPlugins();
  for (int i = 0; i < pluginCount; ++i) {
      RsPlugin *rsPlugin = rsPlugins->plugin(i);
      if (rsPlugin) {
          FeedNotify *feedNotify = rsPlugin->qt_feedNotify();
          if (feedNotify) {
              QString name;
              if (feedNotify->hasSetting(name)) {

                  QCheckBox *enabledCheckBox = new QCheckBox(name, this);
                  enabledCheckBox->setFont(font);
                  ui.pluginFeedNotifyLayout->addWidget(enabledCheckBox, rowFeed++);

                  mFeedNotifySettingList.push_back(FeedNotifySetting(feedNotify, enabledCheckBox));
              }
          }

          ToasterNotify *toasterNotify = rsPlugin->qt_toasterNotify();
          if (toasterNotify) {
              QString name;
              if (toasterNotify->hasSetting(name)) {

                  QCheckBox *enabledCheckBox = new QCheckBox(name, this);
                  enabledCheckBox->setFont(font);
                  ui.pluginToasterNotifyLayout->addWidget(enabledCheckBox, rowToaster++);

                  mToasterNotifySettingList.push_back(ToasterNotifySetting(toasterNotify, enabledCheckBox));
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
                      }
                      ui.pluginToasterNotifyLayout->addWidget(widget, rowToaster++);
                  }
              }
          }
      }
  }

  /* add user notify */
  const QList<UserNotify*> &userNotifyList = MainWindow::getInstance()->getUserNotifyList();
  QList<UserNotify*>::const_iterator it;
  rowFeed = 0;
  mChatLobbyUserNotify = 0;
  for (it = userNotifyList.begin(); it != userNotifyList.end(); ++it) {
      UserNotify *userNotify = *it;

      QString name;
      if (!userNotify->hasSetting(&name, NULL)) {
          continue;
      }

      QCheckBox *enabledCheckBox = new QCheckBox(name, this);
      enabledCheckBox->setFont(font);
      ui.userNotifyLayout->addWidget(enabledCheckBox, rowFeed, 0, 0);
      connect(enabledCheckBox, SIGNAL(toggled(bool)), this, SLOT(notifyToggled()));

      QCheckBox *combinedCheckBox = new QCheckBox(tr("Combined"), this);
      combinedCheckBox->setFont(font);
      ui.userNotifyLayout->addWidget(combinedCheckBox, rowFeed, 1);

      QCheckBox *blinkCheckBox = new QCheckBox(tr("Blink"), this);
      blinkCheckBox->setFont(font);
      ui.userNotifyLayout->addWidget(blinkCheckBox, rowFeed++, 2);

      mUserNotifySettingList.push_back(UserNotifySetting(userNotify, enabledCheckBox, combinedCheckBox, blinkCheckBox));

      //To get ChatLobbyUserNotify Settings
      if (!mChatLobbyUserNotify) mChatLobbyUserNotify = dynamic_cast<ChatLobbyUserNotify*>(*it);
  }
}

NotifyPage::~NotifyPage()
{
}

uint NotifyPage::getNewsFlags()
{
    uint newsFlags = 0;

    if (ui.notify_Peers->isChecked())
        newsFlags |= RS_FEED_TYPE_PEER;
    if (ui.notify_Channels->isChecked())
        newsFlags |= RS_FEED_TYPE_CHANNEL;
    if (ui.notify_Forums->isChecked())
        newsFlags |= RS_FEED_TYPE_FORUM;
    if (ui.notify_Posted->isChecked())
        newsFlags |= RS_FEED_TYPE_POSTED;
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

/** Saves the changes on this page */
bool
NotifyPage::save(QString &/*errmsg*/)
{
    /* extract from rsNotify the flags */

    uint messageflags = 0;

    if (ui.message_ConnectAttempt->isChecked())
        messageflags |= RS_MESSAGE_CONNECT_ATTEMPT;

    /* save feed notify */
    QList<FeedNotifySetting>::iterator feedNotifyIt;
    for (feedNotifyIt = mFeedNotifySettingList.begin(); feedNotifyIt != mFeedNotifySettingList.end(); ++feedNotifyIt) {
        feedNotifyIt->mFeedNotify->setNotifyEnabled(feedNotifyIt->mEnabledCheckBox->isChecked());
    }

    /* save toaster notify */
    QList<ToasterNotifySetting>::iterator toasterNotifyIt;
    for (toasterNotifyIt = mToasterNotifySettingList.begin(); toasterNotifyIt != mToasterNotifySettingList.end(); ++toasterNotifyIt) {
        if(toasterNotifyIt->mEnabledCheckBox->accessibleName().isEmpty()){
            toasterNotifyIt->mToasterNotify->setNotifyEnabled(toasterNotifyIt->mEnabledCheckBox->isChecked()) ;
        } else {
            toasterNotifyIt->mToasterNotify->setNotifyEnabled(toasterNotifyIt->mEnabledCheckBox->accessibleName(), toasterNotifyIt->mEnabledCheckBox->isChecked()) ;
        }
    }

    /* save user notify */
    QList<UserNotifySetting>::iterator notifyIt;
    for (notifyIt = mUserNotifySettingList.begin(); notifyIt != mUserNotifySettingList.end(); ++notifyIt) {
        notifyIt->mUserNotify->setNotifyEnabled(notifyIt->mEnabledCheckBox->isChecked(), notifyIt->mCombinedCheckBox->isChecked(), notifyIt->mBlinkCheckBox->isChecked());
    }

    Settings->setNotifyFlags(getNotifyFlags());
    Settings->setNewsFeedFlags(getNewsFlags());
    Settings->setMessageFlags(messageflags);

    Settings->setDisplayTrayChatLobby(ui.systray_ChatLobby->isChecked());
    Settings->setDisplayTrayGroupChat(ui.systray_GroupChat->isChecked());
    MainWindow::installGroupChatNotifier();
    MainWindow::installNotifyIcons();

    int index = ui.comboBoxToasterPosition->currentIndex();
    if (index != -1) {
        Settings->setToasterPosition((RshareSettings::enumToasterPosition) ui.comboBoxToasterPosition->itemData(index).toInt());
    }

    Settings->setToasterMargin(QPoint(ui.spinBoxToasterXMargin->value(), ui.spinBoxToasterYMargin->value()));

    if (mChatLobbyUserNotify){
        mChatLobbyUserNotify->setCountUnRead(ui.chatLobbies_CountUnRead->isChecked()) ;
        mChatLobbyUserNotify->setCheckForNickName(ui.chatLobbies_CheckNickName->isChecked()) ;
        mChatLobbyUserNotify->setCountSpecificText(ui.chatLobbies_CountFollowingText->isChecked()) ;
        mChatLobbyUserNotify->setTextToNotify(ui.chatLobbies_TextToNotify->document()->toPlainText());
    }
    load();
    return true;
}

/** Loads the settings for this page */
void NotifyPage::load()
{
    /* extract from rsNotify the flags */
    uint notifyflags = Settings->getNotifyFlags();
    uint newsflags = Settings->getNewsFeedFlags();
    uint messageflags = Settings->getMessageFlags();

    ui.popup_Connect->setChecked(notifyflags & RS_POPUP_CONNECT);
    ui.popup_NewMsg->setChecked(notifyflags & RS_POPUP_MSG);
    ui.popup_DownloadFinished->setChecked(notifyflags & RS_POPUP_DOWNLOAD);
    ui.popup_PrivateChat->setChecked(notifyflags & RS_POPUP_CHAT);
    ui.popup_GroupChat->setChecked(notifyflags & RS_POPUP_GROUPCHAT);
    ui.popup_ChatLobby->setChecked(notifyflags & RS_POPUP_CHATLOBBY);
    ui.popup_ConnectAttempt->setChecked(notifyflags & RS_POPUP_CONNECT_ATTEMPT);

    ui.notify_Peers->setChecked(newsflags & RS_FEED_TYPE_PEER);
    ui.notify_Channels->setChecked(newsflags & RS_FEED_TYPE_CHANNEL);
    ui.notify_Forums->setChecked(newsflags & RS_FEED_TYPE_FORUM);
    ui.notify_Posted->setChecked(newsflags & RS_FEED_TYPE_POSTED);
#if 0
    ui.notify_Blogs->setChecked(newsflags & RS_FEED_TYPE_BLOG);
#endif
    ui.notify_Chat->setChecked(newsflags & RS_FEED_TYPE_CHAT);
    ui.notify_Messages->setChecked(newsflags & RS_FEED_TYPE_MSG);
    ui.notify_Chat->setChecked(newsflags & RS_FEED_TYPE_CHAT);
    ui.notify_Security->setChecked(newsflags & RS_FEED_TYPE_SECURITY);
    ui.notify_SecurityIp->setChecked(newsflags & RS_FEED_TYPE_SECURITY_IP);

    ui.message_ConnectAttempt->setChecked(messageflags & RS_MESSAGE_CONNECT_ATTEMPT);

    ui.systray_GroupChat->setChecked(Settings->getDisplayTrayGroupChat());
    ui.systray_ChatLobby->setChecked(Settings->getDisplayTrayChatLobby());

    ui.pushButtonDisableAll->setChecked(NotifyQt::isAllDisable());

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
        ui.comboBoxToasterPosition->addItem(it.value(), it.key());

        if (it.key() == toasterPosition) {
            ui.comboBoxToasterPosition->setCurrentIndex(index);
        }
    }

    QPoint margin = Settings->getToasterMargin();
    ui.spinBoxToasterXMargin->setValue(margin.x());
    ui.spinBoxToasterYMargin->setValue(margin.y());

    /* load feed notify */
    QList<FeedNotifySetting>::iterator feedNotifyIt;
    for (feedNotifyIt = mFeedNotifySettingList.begin(); feedNotifyIt != mFeedNotifySettingList.end(); ++feedNotifyIt) {
        feedNotifyIt->mEnabledCheckBox->setChecked(feedNotifyIt->mFeedNotify->notifyEnabled());
    }

    /* load toaster notify */
    QList<ToasterNotifySetting>::iterator toasterNotifyIt;
    for (toasterNotifyIt = mToasterNotifySettingList.begin(); toasterNotifyIt != mToasterNotifySettingList.end(); ++toasterNotifyIt) {
        if (toasterNotifyIt->mEnabledCheckBox->accessibleName().isEmpty()) {
            toasterNotifyIt->mEnabledCheckBox->setChecked(toasterNotifyIt->mToasterNotify->notifyEnabled()) ;
        } else {
            toasterNotifyIt->mEnabledCheckBox->setChecked(toasterNotifyIt->mToasterNotify->notifyEnabled(toasterNotifyIt->mEnabledCheckBox->accessibleName())) ;
        }
    }

    /* load user notify */
    QList<UserNotifySetting>::iterator userNotifyIt;
    for (userNotifyIt = mUserNotifySettingList.begin(); userNotifyIt != mUserNotifySettingList.end(); ++userNotifyIt) {
        userNotifyIt->mEnabledCheckBox->setChecked(userNotifyIt->mUserNotify->notifyEnabled());
        userNotifyIt->mCombinedCheckBox->setChecked(userNotifyIt->mUserNotify->notifyCombined());
        userNotifyIt->mBlinkCheckBox->setChecked(userNotifyIt->mUserNotify->notifyBlink());
    }

    notifyToggled();

    if (mChatLobbyUserNotify){
        ui.chatLobbies_TextToNotify->setEnabled(mChatLobbyUserNotify->isCountSpecificText()) ;
        ui.chatLobbies_CountFollowingText->setChecked(mChatLobbyUserNotify->isCountSpecificText()) ;
        ui.chatLobbies_CountUnRead->setChecked(mChatLobbyUserNotify->isCountUnRead());
        ui.chatLobbies_CheckNickName->setChecked(mChatLobbyUserNotify->isCheckForNickName());
        ui.chatLobbies_TextToNotify->setPlainText(mChatLobbyUserNotify->textToNotify());
    }
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
