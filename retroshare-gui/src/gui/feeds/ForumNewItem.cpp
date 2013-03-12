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

#include "ForumNewItem.h"
#include "FeedHolder.h"

#include <retroshare/rsforums.h>
#include "gui/forums/CreateForumMsg.h"
#include "gui/RetroShareLink.h"

/****
 * #define DEBUG_ITEM 1
 ****/

/** Constructor */
ForumNewItem::ForumNewItem(FeedHolder *parent, uint32_t feedId, const std::string &forumId, bool isHome, bool isNew)
:QWidget(NULL), mParent(parent), mFeedId(feedId),
	mForumId(forumId), mIsHome(isHome), mIsNew(isNew)
{
  /* Invoke the Qt Designer generated object setup routine */
  setupUi(this);

  setAttribute ( Qt::WA_DeleteOnClose, true );

  /* general ones */
  connect( expandButton, SIGNAL( clicked( void ) ), this, SLOT( toggle ( void ) ) );
  connect( clearButton, SIGNAL( clicked( void ) ), this, SLOT( removeItem ( void ) ) );

  /* specific ones */
  connect( subscribeButton, SIGNAL( clicked( void ) ), this, SLOT( subscribeForum ( void ) ) );
  // To Cheeky to post on a brand new forum....
  connect( postButton, SIGNAL( clicked( void ) ), this, SLOT( postToForum ( void ) ) );

  expandFrame->hide();
  updateItemStatic();
  updateItem();
}


void ForumNewItem::updateItemStatic()
{
	if (!rsForums)
		return;


	/* fill in */
#ifdef DEBUG_ITEM
	std::cerr << "ForumNewItem::updateItemStatic()";
	std::cerr << std::endl;
#endif

	ForumInfo fi;
	if (rsForums->getForumInfo(mForumId, fi))
	{
		RetroShareLink link;
		link.createForum(fi.forumId, "");
		nameLabel->setText(link.toHtml());

		descLabel->setText(QString::fromStdWString(fi.forumDesc));

		if (fi.subscribeFlags & RS_DISTRIB_SUBSCRIBED)
		{
			subscribeButton->setEnabled(false);
			postButton->setEnabled(true);
		}
		else
		{
			subscribeButton->setEnabled(true);
			postButton->setEnabled(false);
		}

		/* should also check the other flags */
	}
	else
	{
		nameLabel->setText(tr("Unknown Forum"));
		titleLabel->setText(tr("New Forum"));
		descLabel->setText("");
	}

	if (mIsNew)
	{
		titleLabel->setText(tr("New Forum"));
	}
	else
	{
		titleLabel->setText(tr("Updated Forum"));
	}

	if (mIsHome)
	{
		/* disable buttons */
		clearButton->setEnabled(false);
	}
}


void ForumNewItem::updateItem()
{
	/* fill in */
#ifdef DEBUG_ITEM
	std::cerr << "ForumNewItem::updateItem()";
	std::cerr << std::endl;
#endif

}

void ForumNewItem::toggle()
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

void ForumNewItem::removeItem()
{
#ifdef DEBUG_ITEM
	std::cerr << "ForumNewItem::removeItem()";
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


void ForumNewItem::unsubscribeForum()
{
#ifdef DEBUG_ITEM
	std::cerr << "ForumNewItem::unsubscribeForum()";
	std::cerr << std::endl;
#endif
	if (rsForums)
	{
		rsForums->forumSubscribe(mForumId, false);
	}
	updateItemStatic();
}


void ForumNewItem::subscribeForum()
{
#ifdef DEBUG_ITEM
	std::cerr << "ForumNewItem::subscribeForum()";
	std::cerr << std::endl;
#endif
	if (rsForums)
	{
		rsForums->forumSubscribe(mForumId, true);
	}
	updateItemStatic();
}

void ForumNewItem::postToForum()
{
#ifdef DEBUG_ITEM
	std::cerr << "ForumNewItem::subscribeForum()";
	std::cerr << std::endl;
#endif
	if (mParent)
	{
		//mParent->openMsg(FEEDHOLDER_MSG_FORUM, mForumId, "");
		CreateForumMsg *cfm = new CreateForumMsg(mForumId, "");
    cfm->show();
	}
}
