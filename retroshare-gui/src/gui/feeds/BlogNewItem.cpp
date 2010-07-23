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

#include "BlogNewItem.h"
#include "FeedHolder.h"

#include "rsiface/rsblogs.h"

#define BLOG_DEFAULT_IMAGE ":/images/hi64-app-kblogger.png"


/****
 * #define DEBUG_ITEM 1
 ****/

/** Constructor */
BlogNewItem::BlogNewItem(FeedHolder *parent, uint32_t feedId, std::string blogId, bool isHome, bool isNew)
:QWidget(NULL), mParent(parent), mFeedId(feedId),
	mBlogId(blogId), mIsHome(isHome), mIsNew(isNew)
{
  /* Invoke the Qt Designer generated object setup routine */
  setupUi(this);

  setAttribute ( Qt::WA_DeleteOnClose, true );

  /* general ones */
  connect( expandButton, SIGNAL( clicked( void ) ), this, SLOT( toggle ( void ) ) );
  connect( clearButton, SIGNAL( clicked( void ) ), this, SLOT( removeItem ( void ) ) );

  /* specific ones */
  connect( subscribeButton, SIGNAL( clicked( void ) ), this, SLOT( subscribeBlog ( void ) ) );

  small();
  updateItemStatic();
  updateItem();
}


void BlogNewItem::updateItemStatic()
{
	if (!rsBlogs)
		return;


	/* fill in */
#ifdef DEBUG_ITEM
	std::cerr << "BlogNewItem::updateItemStatic()";
	std::cerr << std::endl;
#endif

	BlogInfo bi;
	if (rsBlogs->getBlogInfo(mBlogId, bi))
	{
		nameLabel->setText(QString::fromStdWString(bi.blogName));

		descLabel->setText(QString::fromStdWString(bi.blogDesc));
		
		if(bi.pngImageLen != 0){

    QPixmap blogImage;
    blogImage.loadFromData(bi.pngChanImage, bi.pngImageLen, "PNG");
    logo_label->setPixmap(QPixmap(blogImage));
    }else{
    QPixmap defaulImage(BLOG_DEFAULT_IMAGE);
    logo_label->setPixmap(QPixmap(defaulImage));
    }

		if (bi.blogFlags & RS_DISTRIB_SUBSCRIBED)
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
		nameLabel->setText("Unknown Blog");
		titleLabel->setText("Blog ???");
		descLabel->setText("");
	}

	if (mIsNew)
	{
		titleLabel->setText("New Blog");
	}
	else
	{
		titleLabel->setText("Updated Blog");
	}

	if (mIsHome)
	{
		/* disable buttons */
		clearButton->setEnabled(false);
		//gotoButton->setEnabled(false);
	}
}


void BlogNewItem::updateItem()
{
	/* fill in */
#ifdef DEBUG_ITEM
	std::cerr << "BlogNewItem::updateItem()";
	std::cerr << std::endl;
#endif

}


void BlogNewItem::small()
{
	expandFrame->hide();
}

void BlogNewItem::toggle()
{
	if (expandFrame->isHidden())
	{
		expandFrame->show();
		expandButton->setIcon(QIcon(QString(":/images/edit_remove24.png")));
	    expandButton->setToolTip("Hide");
	}
	else
	{
		expandFrame->hide();
		expandButton->setIcon(QIcon(QString(":/images/edit_add24.png")));
	    expandButton->setToolTip("Expand");
	}
}


void BlogNewItem::removeItem()
{
#ifdef DEBUG_ITEM
	std::cerr << "BlogNewItem::removeItem()";
	std::cerr << std::endl;
#endif
	hide();
	if (mParent)
	{
		mParent->deleteFeedItem(this, mFeedId);
	}
}


void BlogNewItem::gotoHome()
{
#ifdef DEBUG_ITEM
	std::cerr << "BlogNewItem::gotoHome()";
	std::cerr << std::endl;
#endif
}

/*********** SPECIFIC FUNCTIOSN ***********************/


void BlogNewItem::unsubscribeBlog()
{
#ifdef DEBUG_ITEM
	std::cerr << "BlogNewItem::unsubscribeBlog()";
	std::cerr << std::endl;
#endif
	if (rsBlogs)
	{
		rsBlogs->blogSubscribe(mBlogId, false);
	}
	updateItemStatic();
}


void BlogNewItem::subscribeBlog()
{
#ifdef DEBUG_ITEM
	std::cerr << "BlogNewItem::subscribeBlog()";
	std::cerr << std::endl;
#endif
	if (rsBlogs)
	{
		rsBlogs->blogSubscribe(mBlogId, true);
	}
	updateItemStatic();
}



