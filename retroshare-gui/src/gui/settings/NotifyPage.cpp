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


/** Constructor */
NotifyPage::NotifyPage(QWidget * parent, Qt::WFlags flags)
  : ConfigPage(parent, flags)
{
  /* Invoke the Qt Designer generated object setup routine */
  ui.setupUi(this);

  connect(ui.trayNotify_PrivateChat, SIGNAL(toggled(bool)), this, SLOT(privatChatToggled()));
  connect(ui.trayNotify_Messages, SIGNAL(toggled(bool)), this, SLOT(privatChatToggled()));
  connect(ui.trayNotify_Channels, SIGNAL(toggled(bool)), this, SLOT(privatChatToggled()));
  connect(ui.trayNotify_Forums, SIGNAL(toggled(bool)), this, SLOT(privatChatToggled()));
  connect(ui.trayNotify_Transfer, SIGNAL(toggled(bool)), this, SLOT(privatChatToggled()));

  /* Hide platform specific features */
#ifdef Q_WS_WIN

#endif
}

NotifyPage::~NotifyPage()
{
}

/** Saves the changes on this page */
bool
NotifyPage::save(QString &/*errmsg*/)
{
    /* extract from rsNotify the flags */

    uint notifyflags = 0;
    uint traynotifyflags = 0;
    uint newsflags   = 0;
    uint chatflags   = 0;

    if (ui.popup_Connect->isChecked())
        notifyflags |= RS_POPUP_CONNECT;
    if (ui.popup_NewMsg->isChecked())
        notifyflags |= RS_POPUP_MSG;
    if (ui.popup_DownloadFinished->isChecked())
        notifyflags |= RS_POPUP_DOWNLOAD;

    if (ui.notify_Peers->isChecked())
        newsflags |= RS_FEED_TYPE_PEER;
    if (ui.notify_Channels->isChecked())
        newsflags |= RS_FEED_TYPE_CHAN;
    if (ui.notify_Forums->isChecked())
        newsflags |= RS_FEED_TYPE_FORUM;
    if (ui.notify_Blogs->isChecked())
        newsflags |= RS_FEED_TYPE_BLOG;
    if (ui.notify_Chat->isChecked())
        newsflags |= RS_FEED_TYPE_CHAT;
    if (ui.notify_Messages->isChecked())
        newsflags |= RS_FEED_TYPE_MSG;
    if (ui.notify_Chat->isChecked())
        newsflags |= RS_FEED_TYPE_CHAT;

    if (ui.chat_NewWindow->isChecked())
        chatflags |= RS_CHAT_OPEN;
    if (ui.chat_Focus->isChecked())
        chatflags |= RS_CHAT_FOCUS;
    if (ui.chat_tabbedWindow->isChecked())
        chatflags |= RS_CHAT_TABBED_WINDOW;

    if (ui.trayNotify_PrivateChat->isChecked())
        traynotifyflags |= TRAYNOTIFY_PRIVATECHAT;
    if (ui.trayNotify_Messages->isChecked())
        traynotifyflags |= TRAYNOTIFY_MESSAGES;
    if (ui.trayNotify_Channels->isChecked())
        traynotifyflags |= TRAYNOTIFY_CHANNELS;
    if (ui.trayNotify_Forums->isChecked())
        traynotifyflags |= TRAYNOTIFY_FORUMS;
    if (ui.trayNotify_Transfer->isChecked())
        traynotifyflags |= TRAYNOTIFY_TRANSFERS;

    if (ui.trayNotify_PrivateChatCombined->isChecked())
        traynotifyflags |= TRAYNOTIFY_PRIVATECHAT_COMBINED;
    if (ui.trayNotify_MessagesCombined->isChecked())
        traynotifyflags |= TRAYNOTIFY_MESSAGES_COMBINED;
    if (ui.trayNotify_ChannelsCombined->isChecked())
        traynotifyflags |= TRAYNOTIFY_CHANNELS_COMBINED;
    if (ui.trayNotify_ForumsCombined->isChecked())
        traynotifyflags |= TRAYNOTIFY_FORUMS_COMBINED;
    if (ui.trayNotify_TransferCombined->isChecked())
        traynotifyflags |= TRAYNOTIFY_TRANSFERS_COMBINED;

    Settings->setNotifyFlags(notifyflags);
    Settings->setTrayNotifyFlags(traynotifyflags);
    Settings->setNewsFeedFlags(newsflags);
    Settings->setChatFlags(chatflags);

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
    uint traynotifyflags = Settings->getTrayNotifyFlags();
    uint newsflags = Settings->getNewsFeedFlags();
    uint chatflags = Settings->getChatFlags();

    ui.popup_Connect->setChecked(notifyflags & RS_POPUP_CONNECT);
    ui.popup_NewMsg->setChecked(notifyflags & RS_POPUP_MSG);
    ui.popup_DownloadFinished->setChecked(notifyflags & RS_POPUP_DOWNLOAD);

    ui.notify_Peers->setChecked(newsflags & RS_FEED_TYPE_PEER);
    ui.notify_Channels->setChecked(newsflags & RS_FEED_TYPE_CHAN);
    ui.notify_Forums->setChecked(newsflags & RS_FEED_TYPE_FORUM);
    ui.notify_Blogs->setChecked(newsflags & RS_FEED_TYPE_BLOG);
    ui.notify_Chat->setChecked(newsflags & RS_FEED_TYPE_CHAT);
    ui.notify_Messages->setChecked(newsflags & RS_FEED_TYPE_MSG);
    ui.notify_Chat->setChecked(newsflags & RS_FEED_TYPE_CHAT);

    ui.chat_NewWindow->setChecked(chatflags & RS_CHAT_OPEN);
    ui.chat_Focus->setChecked(chatflags & RS_CHAT_FOCUS);
    ui.chat_tabbedWindow->setChecked(chatflags & RS_CHAT_TABBED_WINDOW);

    ui.systray_GroupChat->setChecked(Settings->getDisplayTrayGroupChat());

    ui.trayNotify_PrivateChat->setChecked(traynotifyflags & TRAYNOTIFY_PRIVATECHAT);
    ui.trayNotify_Messages->setChecked(traynotifyflags & TRAYNOTIFY_MESSAGES);
    ui.trayNotify_Channels->setChecked(traynotifyflags & TRAYNOTIFY_CHANNELS);
    ui.trayNotify_Forums->setChecked(traynotifyflags & TRAYNOTIFY_FORUMS);
    ui.trayNotify_Transfer->setChecked(traynotifyflags & TRAYNOTIFY_TRANSFERS);

    ui.trayNotify_PrivateChatCombined->setChecked(traynotifyflags & TRAYNOTIFY_PRIVATECHAT_COMBINED);
    ui.trayNotify_MessagesCombined->setChecked(traynotifyflags & TRAYNOTIFY_MESSAGES_COMBINED);
    ui.trayNotify_ChannelsCombined->setChecked(traynotifyflags & TRAYNOTIFY_CHANNELS_COMBINED);
    ui.trayNotify_ForumsCombined->setChecked(traynotifyflags & TRAYNOTIFY_FORUMS_COMBINED);
    ui.trayNotify_TransferCombined->setChecked(traynotifyflags & TRAYNOTIFY_TRANSFERS_COMBINED);

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

    privatChatToggled();
}

void NotifyPage::privatChatToggled()
{
    QList<QPair<QCheckBox*, QCheckBox*> > checkboxes;
    checkboxes << qMakePair(ui.trayNotify_PrivateChat, ui.trayNotify_PrivateChatCombined)
               << qMakePair(ui.trayNotify_Messages, ui.trayNotify_MessagesCombined)
               << qMakePair(ui.trayNotify_Channels, ui.trayNotify_ChannelsCombined)
               << qMakePair(ui.trayNotify_Forums, ui.trayNotify_ForumsCombined)
               << qMakePair(ui.trayNotify_Transfer, ui.trayNotify_TransferCombined);

    QList<QPair<QCheckBox*, QCheckBox*> >::iterator checkboxIt;
    for (checkboxIt = checkboxes.begin(); checkboxIt != checkboxes.end(); checkboxIt++) {
        if (checkboxIt->first->isChecked()) {
            checkboxIt->second->setEnabled(true);
        } else {
            checkboxIt->second->setChecked(false);
            checkboxIt->second->setEnabled(false);
        }
    }
}
