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
#include <QtGui>

#include "SubDestItem.h"
#include "FeedHolder.h"

#include "rsiface/rspeers.h"
#include "rsiface/rsforums.h"
#include "rsiface/rschannels.h"

#include <iostream>

/****
 * #define DEBUG_ITEM 1
 ****/

/** Constructor */
SubDestItem::SubDestItem(uint32_t type, std::string groupId, std::string inReplyTo)
:QWidget(NULL), mType(type), mGroupId(groupId), mInReplyTo(inReplyTo)
{
  /* Invoke the Qt Designer generated object setup routine */
  setupUi(this);

  connect( cancelButton, SIGNAL( clicked( void ) ), this, SLOT( cancel ( void ) ) );

  updateItemStatic();
}

void SubDestItem::updateItemStatic()
{
	/* fill in */
#ifdef DEBUG_ITEM
	std::cerr << "SubDestItem::updateItemStatic()";
	std::cerr << std::endl;
#endif

	QString name = "Unknown";
	switch(mType)
	{
		case FEEDHOLDER_MSG_MESSAGE:
		{
			typeLabel->setText("Message To: ");
			if (rsPeers)
			{
				name = QString::fromStdString(rsPeers->getPeerName(mGroupId));
			}
		}
			break;

		case FEEDHOLDER_MSG_FORUM:
		{
			typeLabel->setText("Forum Post To: ");
			ForumInfo fi;
			if ((rsForums) && (rsForums->getForumInfo(mGroupId, fi)))
			{
				name = QString::fromStdWString(fi.forumName);
			}
		}
			break;

		case FEEDHOLDER_MSG_CHANNEL:
		{
			typeLabel->setText("Channel Post To: ");
			ChannelInfo ci;
			if ((rsChannels) && (rsChannels->getChannelInfo(mGroupId, ci)))
			{
				name = QString::fromStdWString(ci.channelName);
			}
		}
			break;

		case FEEDHOLDER_MSG_BLOG:
		{
			typeLabel->setText("Blog Post");
			name = "";
		}
			break;

	}
	nameLabel->setText(name);
}

void SubDestItem::cancel()
{
#ifdef DEBUG_ITEM
	std::cerr << "SubDestItem::cancel()";
	std::cerr << std::endl;
#endif
	hide();
}

