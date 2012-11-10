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

#include <QDateTime>

#include "BlogsMsgItem.h"
#include "gui/feeds/FeedHolder.h"

#include <retroshare/rsblogs.h>
#include "rshare.h"

/****
 * #define DEBUG_ITEM 1
 ****/

/** Constructor */
BlogsMsgItem::BlogsMsgItem(FeedHolder *parent, uint32_t feedId, std::string peerId, std::string blogId, std::string msgId, bool isHome)
:QWidget(NULL), mParent(parent), mFeedId(feedId),
	mPeerId(peerId), mBlogId(blogId), mMsgId(msgId), mIsHome(isHome)
{
  /* Invoke the Qt Designer generated object setup routine */
  setupUi(this);

  /* general ones */
  connect( expandButton, SIGNAL( clicked( void ) ), this, SLOT( toggle ( void ) ) );
  connect( clearButton, SIGNAL( clicked( void ) ), this, SLOT( removeItem ( void ) ) );
  //connect( gotoButton, SIGNAL( clicked( void ) ), this, SLOT( gotoHome ( void ) ) );

  /* specific ones */
  //connect( playButton, SIGNAL( clicked( void ) ), this, SLOT( playMedia ( void ) ) );
  connect( unsubscribeButton, SIGNAL( clicked( void ) ), this, SLOT( unsubscribeChannel ( void ) ) );

  small();
  updateItemStatic();
  updateItem();

}


void BlogsMsgItem::updateItemStatic()
{
	/* fill in */
#ifdef DEBUG_ITEM
	std::cerr << "ChanMsgItem::updateItemStatic()";
	std::cerr << std::endl;
#endif

	BlogMsgInfo cmi;

	if (!rsBlogs) 
		return;

	if (!rsBlogs->getBlogMessage(mBlogId, mMsgId, cmi))
		return;

	QString title;

	if (!mIsHome)
	{
		BlogInfo ci;
		rsBlogs->getBlogInfo(mBlogId, ci);
		title = "Channel Feed: ";
		title += QString::fromStdWString(ci.blogName);
		titleLabel->setText(title);
		//subjectLabel->setText(QString::fromStdWString(cmi.subject));
	}
	else
	{
		/* subject */
		titleLabel->setText(QString::fromStdWString(cmi.subject));
		/* Blog Message */
		textBrowser->setHtml( QString::fromStdWString(cmi.msg));
	}
	
	//msgLabel->setText(QString::fromStdWString(cmi.msg));
	//msgcommentstextEdit->setHtml(QString::fromStdWString(cmi.msg));

	datetimelabel->setText(Rshare::customDate(cmi.ts));
	
	//playButton->setEnabled(false);
	
	if (mIsHome)
	{
		/* disable buttons */
		clearButton->setEnabled(false);
		//gotoButton->setEnabled(false);
		unsubscribeButton->setEnabled(false);

		clearButton->hide();
	}

	/* don't really want this at all! */
	unsubscribeButton->hide();
	//playButton->hide();
}


void BlogsMsgItem::updateItem()
{
	/* fill in */
#ifdef DEBUG_ITEM
	std::cerr << "BlogMsgItem::updateItem()";
	std::cerr << std::endl;
#endif


}


void BlogsMsgItem::small()
{
	expandFrame->hide();
}

void BlogsMsgItem::toggle()
{
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
}


void BlogsMsgItem::removeItem()
{
#ifdef DEBUG_ITEM
	std::cerr << "ChanMsgItem::removeItem()";
	std::cerr << std::endl;
#endif
	hide();
	if (mParent)
	{
		mParent->deleteFeedItem(this, mFeedId);
	}
}


void BlogsMsgItem::gotoHome()
{
#ifdef DEBUG_ITEM
	std::cerr << "ChanMsgItem::gotoHome()";
	std::cerr << std::endl;
#endif
}

/*********** SPECIFIC FUNCTIOSN ***********************/


void BlogsMsgItem::unsubscribeChannel()
{
#ifdef DEBUG_ITEM
	std::cerr << "ChanMsgItem::unsubscribeChannel()";
	std::cerr << std::endl;
#endif
}


void BlogsMsgItem::playMedia()
{
#ifdef DEBUG_ITEM
	std::cerr << "ChanMsgItem::playMedia()";
	std::cerr << std::endl;
#endif
}


