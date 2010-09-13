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
#include <QTimer>

#include "ChanMsgItem.h"

#include "FeedHolder.h"
#include "SubFileItem.h"

#include <retroshare/rschannels.h>

#include <sstream>


/****
 * #define DEBUG_ITEM 1
 ****/

/** Constructor */
ChanMsgItem::ChanMsgItem(FeedHolder *parent, uint32_t feedId, std::string chanId, std::string msgId, bool isHome)
:QWidget(NULL), mParent(parent), mFeedId(feedId),
	mChanId(chanId), mMsgId(msgId), mIsHome(isHome)
{
  /* Invoke the Qt Designer generated object setup routine */
  setupUi(this);

  setAttribute ( Qt::WA_DeleteOnClose, true );

  /* general ones */
  connect( expandButton, SIGNAL( clicked( void ) ), this, SLOT( toggle ( void ) ) );
  connect( clearButton, SIGNAL( clicked( void ) ), this, SLOT( removeItem ( void ) ) );

  /* specific */
  connect( unsubscribeButton, SIGNAL( clicked( void ) ), this, SLOT( unsubscribeChannel ( void ) ) );


  small();
  updateItemStatic();
  updateItem();

}


void ChanMsgItem::updateItemStatic()
{
	/* fill in */
#ifdef DEBUG_ITEM
	std::cerr << "ChanMsgItem::updateItemStatic()";
	std::cerr << std::endl;
#endif

	ChannelMsgInfo cmi;

	if (!rsChannels) 
		return;

	if (!rsChannels->getChannelMessage(mChanId, mMsgId, cmi))
		return;

	QString title;

	if (!mIsHome)
	{
		ChannelInfo ci;
		rsChannels->getChannelInfo(mChanId, ci);
		title = "Channel Feed: ";
		title += QString::fromStdWString(ci.channelName);
		titleLabel->setText(title);
		subjectLabel->setText(QString::fromStdWString(cmi.subject));
	}
	else
	{
		/* subject */
		titleLabel->setText(QString::fromStdWString(cmi.subject));
		subjectLabel->setText(QString::fromStdWString(cmi.msg));
	}
	
	msgLabel->setText(QString::fromStdWString(cmi.msg));

	QDateTime qtime;
	qtime.setTime_t(cmi.ts);
	QString timestamp = qtime.toString("dd.MMMM yyyy hh:mm:ss");
	datetimelabel->setText(timestamp);
	
	{
		std::ostringstream out;
		out << "(" << cmi.count << " Files)";
		filelabel->setText(QString::fromStdString(out.str()));
	}
		

	std::list<FileInfo>::iterator it;
	for(it = cmi.files.begin(); it != cmi.files.end(); it++)
	{
		/* add file */
		SubFileItem *fi = new SubFileItem(it->hash, it->fname, it->path, it->size,
				SFI_STATE_REMOTE | SFI_TYPE_CHANNEL, mChanId);
		mFileItems.push_back(fi);

		QLayout *layout = expandFrame->layout();
		layout->addWidget(fi);
	}

	if(cmi.thumbnail.image_thumbnail != NULL)
	{
		QPixmap thumbnail;
		thumbnail.loadFromData(cmi.thumbnail.image_thumbnail, cmi.thumbnail.im_thumbnail_size,
				"PNG");

		label->setPixmap(thumbnail);
		label->setStyleSheet("QLabel#label{border: 2px solid #D3D3D3;border-radius: 2px;}");
	}
	
	if (mIsHome)
	{
		/* disable buttons: deletion facility not enabled with cache services yet */
		clearButton->setEnabled(false);
		unsubscribeButton->setEnabled(false);
		clearButton->hide();
		unsubscribeButton->hide();
	}


}


void ChanMsgItem::updateItem()
{
	/* fill in */
#ifdef DEBUG_ITEM
	std::cerr << "ChanMsgItem::updateItem()";
	std::cerr << std::endl;
#endif
	int msec_rate = 10000;

	/* Very slow Tick to check when all files are downloaded */
	std::list<SubFileItem *>::iterator it;
	for(it = mFileItems.begin(); it != mFileItems.end(); it++)
	{
		if (!(*it)->done())
		{
			/* loop again */
	  		QTimer::singleShot( msec_rate, this, SLOT(updateItem( void ) ));
			return;
		}
	}


}


void ChanMsgItem::small()
{
	expandFrame->hide();
}

void ChanMsgItem::toggle()
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


void ChanMsgItem::removeItem()
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


void ChanMsgItem::gotoHome()
{
#ifdef DEBUG_ITEM
	std::cerr << "ChanMsgItem::gotoHome()";
	std::cerr << std::endl;
#endif
}

/*********** SPECIFIC FUNCTIONS ***********************/


void ChanMsgItem::unsubscribeChannel()
{
 #ifdef DEBUG_ITEM
 	std::cerr << "ChanMsgItem::unsubscribeChannel()";
 	std::cerr << std::endl;
 #endif

	if (rsChannels)
	{
		rsChannels->channelSubscribe(mChanId, false);
	}
	updateItemStatic();
}

