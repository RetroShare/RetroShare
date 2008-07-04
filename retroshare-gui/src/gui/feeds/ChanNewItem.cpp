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

#include "ChanNewItem.h"
#include "FeedHolder.h"

#include "rsiface/rschannels.h"

#include <iostream>

#define DEBUG_ITEM 1

/** Constructor */
ChanNewItem::ChanNewItem(FeedHolder *parent, uint32_t feedId, std::string chanId, bool isHome, bool isNew)
:QWidget(NULL), mParent(parent), mFeedId(feedId),
	mChanId(chanId), mIsHome(isHome), mIsNew(isNew)
{
  /* Invoke the Qt Designer generated object setup routine */
  setupUi(this);

  /* general ones */
  connect( expandButton, SIGNAL( clicked( void ) ), this, SLOT( toggle ( void ) ) );
  connect( clearButton, SIGNAL( clicked( void ) ), this, SLOT( removeItem ( void ) ) );
  //connect( gotoButton, SIGNAL( clicked( void ) ), this, SLOT( gotoHome ( void ) ) );

  /* specific ones */
  connect( subscribeButton, SIGNAL( clicked( void ) ), this, SLOT( subscribeChannel ( void ) ) );

  small();
  updateItemStatic();
  updateItem();
}


void ChanNewItem::updateItemStatic()
{
	if (!rsChannels)
		return;


	/* fill in */
#ifdef DEBUG_ITEM
	std::cerr << "ChanNewItem::updateItemStatic()";
	std::cerr << std::endl;
#endif

	ChannelInfo ci;
	if (rsChannels->getChannelInfo(mChanId, ci))
	{
		nameLabel->setText(QString::fromStdWString(ci.channelName));

		descLabel->setText(QString::fromStdWString(ci.channelDesc));

		if (ci.channelFlags & RS_DISTRIB_SUBSCRIBED)
		{
			subscribeButton->setEnabled(false);
			//postButton->setEnabled(true);
		}
		else
		{
			subscribeButton->setEnabled(true);
			//postButton->setEnabled(false);
		}
			

		/* should also check the other flags */
	}
	else
	{
		nameLabel->setText("Unknown Channel");
		titleLabel->setText("Channel ???");
		descLabel->setText("");
	}

	if (mIsNew)
	{
		titleLabel->setText("New Channel");
	}
	else
	{
		titleLabel->setText("Updated Channel");
	}

	if (mIsHome)
	{
		/* disable buttons */
		clearButton->setEnabled(false);
		//gotoButton->setEnabled(false);
	}
}


void ChanNewItem::updateItem()
{
	/* fill in */
#ifdef DEBUG_ITEM
	std::cerr << "ChanNewItem::updateItem()";
	std::cerr << std::endl;
#endif

}


void ChanNewItem::small()
{
	expandFrame->hide();
}

void ChanNewItem::toggle()
{
	if (expandFrame->isHidden())
	{
		expandFrame->show();
	}
	else
	{
		expandFrame->hide();
	}
}


void ChanNewItem::removeItem()
{
#ifdef DEBUG_ITEM
	std::cerr << "ChanNewItem::removeItem()";
	std::cerr << std::endl;
#endif
	hide();
	if (mParent)
	{
		mParent->deleteFeedItem(this, mFeedId);
	}
}


void ChanNewItem::gotoHome()
{
#ifdef DEBUG_ITEM
	std::cerr << "ChanNewItem::gotoHome()";
	std::cerr << std::endl;
#endif
}

/*********** SPECIFIC FUNCTIOSN ***********************/


void ChanNewItem::unsubscribeChannel()
{
#ifdef DEBUG_ITEM
	std::cerr << "ChanNewItem::unsubscribeChannel()";
	std::cerr << std::endl;
#endif
	if (rsChannels)
	{
		rsChannels->channelSubscribe(mChanId, false);
	}
	updateItemStatic();
}


void ChanNewItem::subscribeChannel()
{
#ifdef DEBUG_ITEM
	std::cerr << "ChanNewItem::subscribeChannel()";
	std::cerr << std::endl;
#endif
	if (rsChannels)
	{
		rsChannels->channelSubscribe(mChanId, true);
	}
	updateItemStatic();
}



