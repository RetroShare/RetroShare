/****************************************************************
 *  RShare is distributed under the following license:
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


#include <rshare.h>
#include "NotifyDialog.h"
#include <iostream>
#include <sstream>

#include "rsiface/rsnotify.h"

#include <QTimer>



/** Constructor */
NotifyDialog::NotifyDialog(QWidget *parent)
: ConfigPage(parent)
{
  /* Invoke the Qt Designer generated object setup routine */
  ui.setupUi(this);

 /* Create RshareSettings object */
  _settings = new RshareSettings();


   //QTimer *timer = new QTimer(this);
   //timer->connect(timer, SIGNAL(timeout()), this, SLOT(updateStatus()));
   //timer->start(1000);


  /* Hide platform specific features */
#ifdef Q_WS_WIN

#endif
}

/** Saves the changes on this page */
bool
NotifyDialog::save(QString &errmsg)
{
	/* extract from rsNotify the flags */

	uint notifyflags = 0;
	uint newsflags   = 0;
	uint chatflags   = 0;

	if (ui.popup_Connect->isChecked())
		notifyflags |= RS_POPUP_CONNECT;

	if (ui.popup_NewMsg->isChecked())
		notifyflags |= RS_POPUP_MSG;

	if (ui.popup_NewChat->isChecked())
		notifyflags |= RS_POPUP_CHAT;

	//if (ui.popup_Call->isChecked())
	//	notifyflags |= RS_POPUP_CALL;


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
	if (ui.notify_Downloads->isChecked())
		newsflags |= RS_FEED_TYPE_FILES;

	if (ui.chat_NewWindow->isChecked())
		chatflags |= RS_CHAT_OPEN_NEW;
	if (ui.chat_Reopen->isChecked())
		chatflags |= RS_CHAT_REOPEN;
	if (ui.chat_Focus->isChecked())
		chatflags |= RS_CHAT_FOCUS;

	_settings->setNotifyFlags(notifyflags);
	_settings->setNewsFeedFlags(newsflags);
	_settings->setChatFlags(chatflags);

	load();
 	return true;
}


/** Loads the settings for this page */
void NotifyDialog::load()
{
	/* extract from rsNotify the flags */

	uint notifyflags = _settings->getNotifyFlags();
	uint newsflags = _settings->getNewsFeedFlags();
	uint chatflags   = _settings->getChatFlags();

	ui.popup_Connect->setChecked(notifyflags & RS_POPUP_CONNECT);
	ui.popup_NewMsg->setChecked(notifyflags & RS_POPUP_MSG);
	ui.popup_NewChat->setChecked(notifyflags & RS_POPUP_CHAT);
	//ui.popup_Call->setChecked(notifyflags & RS_POPUP_CALL);

	ui.notify_Peers->setChecked(newsflags & RS_FEED_TYPE_PEER);
	ui.notify_Channels->setChecked(newsflags & RS_FEED_TYPE_CHAN);
	ui.notify_Forums->setChecked(newsflags & RS_FEED_TYPE_FORUM);
	ui.notify_Blogs->setChecked(newsflags & RS_FEED_TYPE_BLOG);
	ui.notify_Chat->setChecked(newsflags & RS_FEED_TYPE_CHAT);
	ui.notify_Messages->setChecked(newsflags & RS_FEED_TYPE_MSG);
	ui.notify_Downloads->setChecked(newsflags & RS_FEED_TYPE_FILES);

	ui.chat_NewWindow->setChecked(chatflags & RS_CHAT_OPEN_NEW);
	ui.chat_Reopen->setChecked(chatflags & RS_CHAT_REOPEN);
	ui.chat_Focus->setChecked(chatflags & RS_CHAT_FOCUS);


	/* disable ones that don't work yet */
	ui.notify_Chat->setEnabled(false);
	ui.notify_Blogs->setEnabled(false);
	ui.notify_Downloads->setEnabled(false);
	ui.popup_NewChat->setEnabled(false);
}


/** Loads the settings for this page */
void NotifyDialog::updateStatus()
{

}



