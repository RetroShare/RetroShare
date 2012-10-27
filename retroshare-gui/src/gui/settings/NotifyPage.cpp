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
#include "rsharesettings.h"

#include "gui/MainWindow.h"
#include "gui/common/UserNotify.h"
#include "gui/notifyqt.h"
#include "gui/NewsFeed.h"

/** Constructor */
NotifyPage::NotifyPage(QWidget * parent, Qt::WFlags flags)
  : ConfigPage(parent, flags)
{
  /* Invoke the Qt Designer generated object setup routine */
  ui.setupUi(this);

  connect(ui.notifyButton, SIGNAL(clicked()), this, SLOT(testNotify()));
  connect(ui.toasterButton, SIGNAL(clicked()), this, SLOT(testToaster()));

  /* add user notify */
  QFont font = ui.notify_Peers->font(); // use font from existing checkbox
  const QList<UserNotify*> &userNotifyList = MainWindow::getInstance()->getUserNotifyList();
  QList<UserNotify*>::const_iterator it;
  int row = 0;
  for (it = userNotifyList.begin(); it != userNotifyList.end(); ++it) {
      UserNotify *userNotify = *it;

      QString name;
      if (!userNotify->hasSetting(name)) {
          continue;
      }

      QCheckBox *enabledCheckBox = new QCheckBox(name, this);
      enabledCheckBox->setFont(font);
      ui.notifyLayout->addWidget(enabledCheckBox, row, 0, 0);
      connect(enabledCheckBox, SIGNAL(toggled(bool)), this, SLOT(notifyToggled()));

      QCheckBox *combinedCheckBox = new QCheckBox(tr("Combined"), this);
      combinedCheckBox->setFont(font);
      ui.notifyLayout->addWidget(combinedCheckBox, row, 1);

      QCheckBox *blinkCheckBox = new QCheckBox(tr("Blink"), this);
      blinkCheckBox->setFont(font);
      ui.notifyLayout->addWidget(blinkCheckBox, row++, 2);

      mUserNotifySettingList.push_back(UserNotifySetting(userNotify, enabledCheckBox, combinedCheckBox, blinkCheckBox));
  }

  /* Hide platform specific features */
#ifdef Q_WS_WIN

#endif
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
        newsFlags |= RS_FEED_TYPE_CHAN;
    if (ui.notify_Forums->isChecked())
        newsFlags |= RS_FEED_TYPE_FORUM;
    if (ui.notify_Blogs->isChecked())
        newsFlags |= RS_FEED_TYPE_BLOG;
    if (ui.notify_Messages->isChecked())
        newsFlags |= RS_FEED_TYPE_MSG;
    if (ui.notify_Chat->isChecked())
        newsFlags |= RS_FEED_TYPE_CHAT;
    if (ui.notify_Security->isChecked())
        newsFlags |= RS_FEED_TYPE_SECURITY;

    return newsFlags;
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

    Settings->setAddFeedsAtEnd(ui.addFeedsAtEnd->isChecked());

    int index = ui.comboBoxToasterPosition->currentIndex();
    if (index != -1) {
        Settings->setToasterPosition((RshareSettings::enumToasterPosition) ui.comboBoxToasterPosition->itemData(index).toInt());
    }

    Settings->setToasterMargin(QPoint(ui.spinBoxToasterXMargin->value(), ui.spinBoxToasterYMargin->value()));

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
    ui.notify_Channels->setChecked(newsflags & RS_FEED_TYPE_CHAN);
    ui.notify_Forums->setChecked(newsflags & RS_FEED_TYPE_FORUM);
    ui.notify_Blogs->setChecked(newsflags & RS_FEED_TYPE_BLOG);
    ui.notify_Chat->setChecked(newsflags & RS_FEED_TYPE_CHAT);
    ui.notify_Messages->setChecked(newsflags & RS_FEED_TYPE_MSG);
    ui.notify_Chat->setChecked(newsflags & RS_FEED_TYPE_CHAT);
    ui.notify_Security->setChecked(newsflags & RS_FEED_TYPE_SECURITY);

    ui.message_ConnectAttempt->setChecked(messageflags & RS_MESSAGE_CONNECT_ATTEMPT);

    ui.systray_GroupChat->setChecked(Settings->getDisplayTrayGroupChat());
    ui.systray_ChatLobby->setChecked(Settings->getDisplayTrayChatLobby());

    ui.addFeedsAtEnd->setChecked(Settings->getAddFeedsAtEnd());

    RshareSettings::enumToasterPosition toasterPosition = Settings->getToasterPosition();
    ui.comboBoxToasterPosition->clear();

    QMap<int, QString> toasterPositions;
    toasterPositions[RshareSettings::TOASTERPOS_TOPLEFT] = tr("Top Left");
    toasterPositions[RshareSettings::TOASTERPOS_TOPRIGHT] = tr("Top Right");
    toasterPositions[RshareSettings::TOASTERPOS_BOTTOMLEFT] = tr("Bottom Left");
    toasterPositions[RshareSettings::TOASTERPOS_BOTTOMRIGHT] = tr("Bottom Right");

    QMap<int, QString>::iterator it;
    int index = 0;
    for (it = toasterPositions.begin(); it != toasterPositions.end(); it++, index++) {
        ui.comboBoxToasterPosition->addItem(it.value(), it.key());

        if (it.key() == toasterPosition) {
            ui.comboBoxToasterPosition->setCurrentIndex(index);
        }
    }

    QPoint margin = Settings->getToasterMargin();
    ui.spinBoxToasterXMargin->setValue(margin.x());
    ui.spinBoxToasterYMargin->setValue(margin.y());

    /* load user notify */
    QList<UserNotifySetting>::iterator notifyIt;
    for (notifyIt = mUserNotifySettingList.begin(); notifyIt != mUserNotifySettingList.end(); ++notifyIt) {
        notifyIt->mEnabledCheckBox->setChecked(notifyIt->mUserNotify->notifyEnabled());
        notifyIt->mCombinedCheckBox->setChecked(notifyIt->mUserNotify->notifyCombined());
        notifyIt->mBlinkCheckBox->setChecked(notifyIt->mUserNotify->notifyBlink());
    }

    notifyToggled();
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

void NotifyPage::testNotify()
{
    NewsFeed::testFeeds(getNewsFlags());
}

void NotifyPage::testToaster()
{
    NotifyQt::getInstance()->testToaster(getNotifyFlags(), (RshareSettings::enumToasterPosition) ui.comboBoxToasterPosition->itemData(ui.comboBoxToasterPosition->currentIndex()).toInt(), QPoint(ui.spinBoxToasterXMargin->value(), ui.spinBoxToasterYMargin->value()));
}
