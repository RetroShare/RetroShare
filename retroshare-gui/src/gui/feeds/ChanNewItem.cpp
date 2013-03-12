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

#include "ChanNewItem.h"
#include "FeedHolder.h"
#include "gui/RetroShareLink.h"

#include <retroshare/rschannels.h>

#define CHAN_DEFAULT_IMAGE ":/images/channels.png"

/****
 * #define DEBUG_ITEM 1
 ****/

/** Constructor */
ChanNewItem::ChanNewItem(FeedHolder *parent, uint32_t feedId, const std::string &chanId, bool isHome, bool isNew)
:QWidget(NULL), mParent(parent), mFeedId(feedId),
	mChanId(chanId), mIsHome(isHome), mIsNew(isNew)
{
  /* Invoke the Qt Designer generated object setup routine */
  setupUi(this);

  setAttribute ( Qt::WA_DeleteOnClose, true );

  /* general ones */
  connect( expandButton, SIGNAL( clicked( void ) ), this, SLOT( toggle ( void ) ) );
  connect( clearButton, SIGNAL( clicked( void ) ), this, SLOT( removeItem ( void ) ) );

  /* specific ones */
  connect( subscribeButton, SIGNAL( clicked( void ) ), this, SLOT( subscribeChannel ( void ) ) );

  expandFrame->hide();
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
		RetroShareLink link;
		link.createChannel(ci.channelId, "");
		nameLabel->setText(link.toHtml());

		descLabel->setText(QString::fromStdWString(ci.channelDesc));
		
		if(ci.pngImageLen != 0){
			QPixmap chanImage;
			chanImage.loadFromData(ci.pngChanImage, ci.pngImageLen, "PNG");
			logoLabel->setPixmap(QPixmap(chanImage));
		} else {
			QPixmap defaulImage(CHAN_DEFAULT_IMAGE);
			logoLabel->setPixmap(QPixmap(defaulImage));
		}

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
		nameLabel->setText(tr("Unknown Channel"));
		titleLabel->setText("Channel ???");
		descLabel->setText("");
	}

	if (mIsNew)
	{
		titleLabel->setText(tr("New Channel"));
	}
	else
	{
		titleLabel->setText(tr("Updated Channel"));
	}

	if (mIsHome)
	{
		/* disable buttons */
		clearButton->setEnabled(false);
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

void ChanNewItem::toggle()
{
	mParent->lockLayout(this, true);

	if (expandFrame->isHidden())
	{
		expandFrame->show();
		expandButton->setIcon(QIcon(QString(":/images/edit_remove24.png")));
	    expandButton->setToolTip(tr("Hide"));
	}
	else
	{
		expandFrame->hide();
		expandButton->setIcon(QIcon(QString(":/images/edit_add24.png")));
	    expandButton->setToolTip(tr("Expand"));
	}

	mParent->lockLayout(this, false);
}


void ChanNewItem::removeItem()
{
#ifdef DEBUG_ITEM
	std::cerr << "ChanNewItem::removeItem()";
	std::cerr << std::endl;
#endif

	mParent->lockLayout(this, true);
	hide();
	mParent->lockLayout(this, false);

	if (mParent)
	{
		mParent->deleteFeedItem(this, mFeedId);
	}
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
		rsChannels->channelSubscribe(mChanId, false, false);
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
		rsChannels->channelSubscribe(mChanId, true, true);
	}
	updateItemStatic();
}



