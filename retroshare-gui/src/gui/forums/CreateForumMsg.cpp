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

#include <gui/Preferences/rsharesettings.h>

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
  connect( ui.postmessage_action, SIGNAL( triggered (bool) ), this, SLOT( cancelMsg( ) ) );
  connect( ui.close_action, SIGNAL( triggered (bool) ), this, SLOT( createMsg( ) ) );

  newMsg();

}


void  CreateForumMsg::newMsg()
{
	/* clear all */
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

