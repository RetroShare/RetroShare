/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2008 Robert Fernie
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


#include "CreateForumMsg.h"

#include <gui/settings/rsharesettings.h>

#include "rsiface/rsforums.h"

/** Constructor */
CreateForumMsg::CreateForumMsg(std::string fId, std::string pId)
: QMainWindow(NULL), mForumId(fId), mParentId(pId)
{
  /* Invoke the Qt Designer generated object setup routine */
  ui.setupUi(this);
  
  RshareSettings config;
  config.loadWidgetInformation(this);
  
  // connect up the buttons.
  connect( ui.postmessage_action, SIGNAL( triggered (bool) ), this, SLOT( createMsg( ) ) );
  connect( ui.close_action, SIGNAL( triggered (bool) ), this, SLOT( cancelMsg( ) ) );

  newMsg();

}


void  CreateForumMsg::newMsg()
{
	/* clear all */
	ForumInfo fi;
	if (rsForums->getForumInfo(mForumId, fi))
	{
		ForumMsgInfo msg;

		QString name = QString::fromStdWString(fi.forumName);
		QString subj;
		if ((mParentId != "") && (rsForums->getForumMessage(
				mForumId, mParentId, msg)))
		{
			name += " In Reply to: ";
			name += QString::fromStdWString(msg.title);
			subj = "Re: " + QString::fromStdWString(msg.title);
		}

		ui.forumName->setText(name);
		ui.forumSubject->setText(subj);

		if (fi.forumFlags & RS_DISTRIB_AUTHEN_REQ)
		{
			ui.signBox->setChecked(true);
			//ui.signBox->setEnabled(false);
			// For Testing.
			ui.signBox->setEnabled(true);
		}
		else
		{
			ui.signBox->setEnabled(true);
		}
	}

	ui.forumMessage->setText("");
}

void  CreateForumMsg::createMsg()
{
	QString name = ui.forumSubject->text();
	QString desc = ui.forumMessage->toHtml();


	ForumMsgInfo msgInfo;

	msgInfo.forumId = mForumId;
	msgInfo.threadId = "";
	msgInfo.parentId = mParentId;
	msgInfo.msgId = "";

	msgInfo.title = name.toStdWString();
	msgInfo.msg = desc.toStdWString();

	if (ui.signBox->isChecked())
	{
		msgInfo.msgflags = RS_DISTRIB_AUTHEN_REQ;
	}

	if ((msgInfo.msg == L"") && (msgInfo.title == L""))
		return; /* do nothing */

	rsForums->ForumMessageSend(msgInfo);

	close();
	return;
}


void  CreateForumMsg::cancelMsg()
{
	close();
	return;
	        
	RshareSettings config;
	config.saveWidgetInformation(this);
}

