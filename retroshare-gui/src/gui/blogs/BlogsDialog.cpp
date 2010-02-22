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
#include <QContextMenuEvent>
#include <QCursor>
#include <QMenu>
#include <QMouseEvent>
#include <QPixmap>
#include <QPoint>

#include <iostream>
#include <algorithm>

#include "rsiface/rsblogs.h"
#include "rsiface/rspeers.h" //to retrieve peer/usrId info

#include "BlogsDialog.h"

#include "BlogsMsgItem.h"
#include "CreateBlog.h"
#include "CreateBlogMsg.h"

#include "gui/ChanGroupDelegate.h"
#include "gui/GeneralMsgDialog.h"


/** Constructor */
BlogsDialog::BlogsDialog(QWidget *parent)
: MainPage (parent)
{
  	/* Invoke the Qt Designer generated object setup routine */
  	setupUi(this);

  	connect(actionCreate_Channel, SIGNAL(triggered()), this, SLOT(createChannel()));
  	connect(postButton, SIGNAL(clicked()), this, SLOT(createMsg()));
  	connect(subscribeButton, SIGNAL( clicked( void ) ), this, SLOT( subscribeChannel ( void ) ) );
  	connect(unsubscribeButton, SIGNAL( clicked( void ) ), this, SLOT( unsubscribeChannel ( void ) ) );


  	mBlogId = "";
  	mPeerId = rsPeers->getOwnId(); // add your id

    model = new QStandardItemModel(0, 2, this);
    model->setHeaderData(0, Qt::Horizontal, tr("Name"), Qt::DisplayRole);
    model->setHeaderData(1, Qt::Horizontal, tr("ID"), Qt::DisplayRole);

    treeView->setModel(model);
    treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);

    treeView->setItemDelegate(new ChanGroupDelegate());
    //treeView->setRootIsDecorated(false);

    // hide header and id column
    treeView->setHeaderHidden(true);
    treeView->hideColumn(1);
	
    QStandardItem *item1 = new QStandardItem(tr("Own Blogs"));
    QStandardItem *item2 = new QStandardItem(tr("Subscribed Blogs"));
    QStandardItem *item3 = new QStandardItem(tr("Popular Blogs"));
    QStandardItem *item4 = new QStandardItem(tr("Other Blogs"));

    model->appendRow(item1);
    model->appendRow(item2);
    model->appendRow(item3);
    model->appendRow(item4);

    connect(treeView, SIGNAL(clicked(const QModelIndex &)), this, SLOT(selectChannel(const QModelIndex &)));
    connect(treeView, SIGNAL(activated(const QModelIndex &)), this, SLOT(toggleSelection(const QModelIndex &)));
    connect(treeView, SIGNAL(customContextMenuRequested( QPoint ) ), this, SLOT( channelListCustomPopupMenu( QPoint ) ) );

    //added from ahead
    updateChannelList();

    mChannelFont = QFont("MS SANS SERIF", 22);
    nameLabel->setFont(mChannelFont);
    
    nameLabel->setMinimumWidth(20);
    
    itemFont = QFont("ARIAL", 10);
    itemFont.setBold(true);
    item1->setFont(itemFont);
    item2->setFont(itemFont);
    item3->setFont(itemFont);
    item4->setFont(itemFont);
		  
    item1->setForeground(QBrush(QColor(79, 79, 79)));
    item2->setForeground(QBrush(QColor(79, 79, 79)));
    item3->setForeground(QBrush(QColor(79, 79, 79)));
    item4->setForeground(QBrush(QColor(79, 79, 79)));
  
    QMenu *channelmenu = new QMenu();
    channelmenu->addAction(actionCreate_Channel); 
    channelmenu->addSeparator();
    channelpushButton->setMenu(channelmenu);

	
    QTimer *timer = new QTimer(this);
    timer->connect(timer, SIGNAL(timeout()), this, SLOT(checkUpdate()));
    timer->start(1000);
}

void BlogsDialog::channelListCustomPopupMenu( QPoint point )
{
      QMenu contextMnu( this );
      QMouseEvent *mevent = new QMouseEvent( QEvent::MouseButtonPress, point, Qt::RightButton, Qt::RightButton, Qt::NoModifier );
      
      QAction *channeldetailsAct = new QAction(QIcon(":/images/info16.png"), tr( "Show Channel Details" ), this );
      connect( channeldetailsAct , SIGNAL( triggered() ), this, SLOT( showChannelDetails() ) );

      contextMnu.clear();
      contextMnu.addAction( channeldetailsAct );

      contextMnu.exec( mevent->globalPos() );


}

void BlogsDialog::createChannel()
{
	CreateBlog *cf = new CreateBlog(NULL, false);

	cf->setWindowTitle(tr("Create a new Blog"));
	cf->show();
}

void BlogsDialog::channelSelection()
{
	/* which item was selected? */


	/* update mBlogId */

	updateChannelMsgs();
}


/*************************************************************************************/
/*************************************************************************************/
/*************************************************************************************/

void BlogsDialog::deleteFeedItem(QWidget *item, uint32_t type)
{
	return;
}

void BlogsDialog::openChat(std::string peerId)
{
	return;
}

void BlogsDialog::openMsg(uint32_t type, std::string grpId, std::string inReplyTo)
{
#ifdef CHAN_DEBUG
	std::cerr << "BlogsDialog::openMsg()";
	std::cerr << std::endl;
#endif
	GeneralMsgDialog *msgDialog = new GeneralMsgDialog(NULL);


	msgDialog->addDestination(type, grpId, inReplyTo);

	msgDialog->show();
	return;
}


void BlogsDialog::createMsg()
{
	if (mBlogId == "")
	{
	return;
	}

	CreateBlogMsg *msgDialog = new CreateBlogMsg(mBlogId);

	msgDialog->show();
	return;
}

void BlogsDialog::selectChannel( std::string cId)
{
	mBlogId = cId;

	updateChannelMsgs();
}

void BlogsDialog::selectChannel(const QModelIndex &index)
{
	int row = index.row();
	int col = index.column();
	if (col != 1) {
		QModelIndex sibling = index.sibling(row, 1);
		if (sibling.isValid())
			mBlogId = sibling.data().toString().toStdString();
	} else
		mBlogId = index.data().toString().toStdString();
	updateChannelMsgs();
}

void BlogsDialog::checkUpdate()
{
	std::list<std::string> blogIds;
	std::list<std::string>::iterator it;
	if (!rsBlogs)
		return;

	if (rsBlogs->blogsChanged(blogIds))
	{
		/* update Forums List */
		updateChannelList();

		it = std::find(blogIds.begin(), blogIds.end(), mBlogId);
		if (it != blogIds.end())
		{
			updateChannelMsgs();
		}
	}
}


void BlogsDialog::updateChannelList()
{

	std::list<BlogInfo> channelList;
	std::list<BlogInfo>::iterator it;
	if (!rsBlogs)
	{
		return;
	}

	rsBlogs->getBlogList(channelList);

	/* get the ids for our lists */
	std::list<std::string> adminIds;
	std::list<std::string> subIds;
	std::list<std::string> popIds;
	std::list<std::string> otherIds;
	std::multimap<uint32_t, std::string> popMap;

	for(it = channelList.begin(); it != channelList.end(); it++)
	{
		/* sort it into Publish (Own), Subscribed, Popular and Other */
		uint32_t flags = it->blogFlags;

		if (flags & RS_DISTRIB_ADMIN)
		{
			adminIds.push_back(it->blogId);
		}
		else if (flags & RS_DISTRIB_SUBSCRIBED)
		{
			subIds.push_back(it->blogId);
		}
		else
		{
			/* rate the others by popularity */
			popMap.insert(std::make_pair(it->pop, it->blogId));
		}
	}

	/* iterate backwards through popMap - take the top 5 or 10% of list */
	uint32_t popCount = 5;
	if (popCount < popMap.size() / 10)
	{
		popCount = popMap.size() / 10;
	}

	uint32_t i = 0;
	uint32_t popLimit = 0;
	std::multimap<uint32_t, std::string>::reverse_iterator rit;
	for(rit = popMap.rbegin(); ((rit != popMap.rend()) && (i < popCount)); rit++, i++)
	{
		popIds.push_back(rit->second);
	}

	if (rit != popMap.rend())
	{
		popLimit = rit->first;
	}

	for(it = channelList.begin(); it != channelList.end(); it++)
	{
		/* ignore the ones we've done already */
		uint32_t flags = it->blogFlags;

		if (flags & RS_DISTRIB_ADMIN)
		{
			continue;
		}
		else if (flags & RS_DISTRIB_SUBSCRIBED)
		{
			continue;
		}
		else
		{
			if (it->pop < popLimit)
			{
				otherIds.push_back(it->blogId);
			}
		}
	}

	/* now we have our lists ---> update entries */

	updateChannelListOwn(adminIds);
	updateChannelListSub(subIds);
	updateChannelListPop(popIds);
	updateChannelListOther(otherIds);
}

void BlogsDialog::updateChannelListOwn(std::list<std::string> &ids)
{
	std::list<std::string>::iterator iit;

	/* remove rows with groups before adding new ones */
	model->item(OWN)->removeRows(0, model->item(OWN)->rowCount());

	for (iit = ids.begin(); iit != ids.end(); iit ++) {
#ifdef CHAN_DEBUG
		std::cerr << "BlogsDialog::updateChannelListOwn(): " << *iit << std::endl;
#endif
		QStandardItem *ownGroup = model->item(OWN);
		QList<QStandardItem *> channel;
		QStandardItem *item1 = new QStandardItem();
		QStandardItem *item2 = new QStandardItem();

		BlogInfo ci;
		if (rsBlogs && rsBlogs->getBlogInfo(*iit, ci)) {
			item1->setData(QVariant(QString::fromStdWString(ci.blogName)), Qt::DisplayRole);
			item2->setData(QVariant(QString::fromStdString(ci.blogId)), Qt::DisplayRole);
			item1->setToolTip(tr("Popularity: %1\nFetches: %2\nAvailable: %3"
					).arg(QString::number(ci.pop)).arg(9999).arg(9999));
		} else {
			item1->setData(QVariant(QString("Unknown Blog")), Qt::DisplayRole);
			item2->setData(QVariant(QString::fromStdString(*iit)), Qt::DisplayRole);
			item1->setToolTip("Unknown Blog\nNo Description");
		}

		channel.append(item1);
		channel.append(item2);
		ownGroup->appendRow(channel);
	}
}

void BlogsDialog::updateChannelListSub(std::list<std::string> &ids)
{
	std::list<std::string>::iterator iit;

	/* remove rows with groups before adding new ones */
	model->item(SUBSCRIBED)->removeRows(0, model->item(SUBSCRIBED)->rowCount());

	for (iit = ids.begin(); iit != ids.end(); iit ++) {
#ifdef CHAN_DEBUG
		std::cerr << "BlogsDialog::updateChannelListSub(): " << *iit << std::endl;
#endif
		QStandardItem *ownGroup = model->item(SUBSCRIBED);
		QList<QStandardItem *> channel;
		QStandardItem *item1 = new QStandardItem();
		QStandardItem *item2 = new QStandardItem();

		BlogInfo ci;
		if (rsBlogs && rsBlogs->getBlogInfo(*iit, ci)) {
			item1->setData(QVariant(QString::fromStdWString(ci.blogName)), Qt::DisplayRole);
			item2->setData(QVariant(QString::fromStdString(ci.blogId)), Qt::DisplayRole);
			item1->setToolTip(tr("Popularity: %1\nFetches: %2\nAvailable: %3"
					).arg(QString::number(ci.pop)).arg(9999).arg(9999));
		} else {
			item1->setData(QVariant(QString("Unknown Channel")), Qt::DisplayRole);
			item2->setData(QVariant(QString::fromStdString(*iit)), Qt::DisplayRole);
			item1->setToolTip("Unknown Channel\nNo Description");
		}

		channel.append(item1);
		channel.append(item2);
		ownGroup->appendRow(channel);
	}

}

void BlogsDialog::updateChannelListPop(std::list<std::string> &ids)
{
	std::list<std::string>::iterator iit;

	/* remove rows with groups before adding new ones */
	model->item(POPULAR)->removeRows(0, model->item(POPULAR)->rowCount());

	for (iit = ids.begin(); iit != ids.end(); iit ++) {
#ifdef CHAN_DEBUG
		std::cerr << "BlogsDialog::updateChannelListPop(): " << *iit << std::endl;
#endif
		QStandardItem *ownGroup = model->item(POPULAR);
		QList<QStandardItem *> channel;
		QStandardItem *item1 = new QStandardItem();
		QStandardItem *item2 = new QStandardItem();

		BlogInfo ci;
		if (rsBlogs && rsBlogs->getBlogInfo(*iit, ci)) {
			item1->setData(QVariant(QString::fromStdWString(ci.blogName)), Qt::DisplayRole);
			item2->setData(QVariant(QString::fromStdString(ci.blogId)), Qt::DisplayRole);
			item1->setToolTip(tr("Popularity: %1\nFetches: %2\nAvailable: %3"
					).arg(QString::number(ci.pop)).arg(9999).arg(9999));
		} else {
			item1->setData(QVariant(QString("Unknown Channel")), Qt::DisplayRole);
			item2->setData(QVariant(QString::fromStdString(*iit)), Qt::DisplayRole);
			item1->setToolTip("Unknown Channel\nNo Description");
		}

		channel.append(item1);
		channel.append(item2);
		ownGroup->appendRow(channel);
	}
}

void BlogsDialog::updateChannelListOther(std::list<std::string> &ids)
{
	std::list<std::string>::iterator iit;

	/* remove rows with groups before adding new ones */
	model->item(OTHER)->removeRows(0, model->item(OTHER)->rowCount());

	for (iit = ids.begin(); iit != ids.end(); iit ++) {
#ifdef CHAN_DEBUG
		std::cerr << "BlogsDialog::updateChannelListOther(): " << *iit << std::endl;
#endif
		QStandardItem *ownGroup = model->item(OTHER);
		QList<QStandardItem *> channel;
		QStandardItem *item1 = new QStandardItem();
		QStandardItem *item2 = new QStandardItem();

		BlogInfo ci;
		if (rsBlogs && rsBlogs->getBlogInfo(*iit, ci)) {
			item1->setData(QVariant(QString::fromStdWString(ci.blogName)), Qt::DisplayRole);
			item2->setData(QVariant(QString::fromStdString(ci.blogId)), Qt::DisplayRole);
			item1->setToolTip(tr("Popularity: %1\nFetches: %2\nAvailable: %3"
					).arg(QString::number(ci.pop)).arg(9999).arg(9999));
		} else {
			item1->setData(QVariant(QString("Unknown Blog")), Qt::DisplayRole);
			item2->setData(QVariant(QString::fromStdString(*iit)), Qt::DisplayRole);
			item1->setToolTip("Unknown Blog\nNo Description");
		}

		channel.append(item1);
		channel.append(item2);
		ownGroup->appendRow(channel);
	}
}

void BlogsDialog::updateChannelMsgs()
{
	if (!rsBlogs)
		return;

	BlogInfo ci;
	if (!rsBlogs->getBlogInfo(mBlogId, ci))
	{
		postButton->setEnabled(false);
		subscribeButton->setEnabled(false);
		unsubscribeButton->setEnabled(false);
		nameLabel->setText("No Blog Selected");
		iconLabel->setEnabled(false);
		return;
	}
	
	iconLabel->setEnabled(true);
	
	/* set textcolor for Blog name  */
	QString channelStr("<span style=\"font-size:22pt; font-weight:500;"
                               "color:white;\">%1</span>");
	
	/* set Blog name */
	QString cname = QString::fromStdWString(ci.blogName);
  nameLabel->setText(channelStr.arg(cname));

	/* do buttons */
	if (ci.blogFlags & RS_DISTRIB_SUBSCRIBED)
	{
		subscribeButton->setEnabled(false);
		unsubscribeButton->setEnabled(true);
	}
	else
	{
		subscribeButton->setEnabled(true);
		unsubscribeButton->setEnabled(false);
	}

	if (ci.blogFlags & RS_DISTRIB_PUBLISH)
	{
		postButton->setEnabled(true);
	}
	else
	{
		postButton->setEnabled(false);
	}

	/* replace all the messages with new ones */
	std::list<BlogsMsgItem *>::iterator mit;
	for(mit = mBlogMsgItems.begin(); mit != mBlogMsgItems.end(); mit++)
	{
		delete (*mit);
	}
	mBlogMsgItems.clear();

	std::list<BlogMsgSummary> msgs;
	std::list<BlogMsgSummary>::iterator it;

	rsBlogs->getBlogMsgList(mBlogId, msgs);

	for(it = msgs.begin(); it != msgs.end(); it++)
	{
		BlogsMsgItem *cmi = new BlogsMsgItem(this, 0, mPeerId, mBlogId, it->msgId, true);

		mBlogMsgItems.push_back(cmi);
		verticalLayout_2->addWidget(cmi);
	}
}


void BlogsDialog::unsubscribeChannel()
{
#ifdef BLOG_DEBUG
	std::cerr << "BlogsDialog::unsubscribeChannel()";
	std::cerr << std::endl;
#endif
	if (rsBlogs)
	{
		rsBlogs->blogSubscribe(mBlogId, false);
	}
	updateChannelMsgs();
}


void BlogsDialog::subscribeChannel()
{
#ifdef BLOG_DEBUG
	std::cerr << "BlogsDialog::subscribeChannel()";
	std::cerr << std::endl;
#endif
	if (rsBlogs)
	{
		rsBlogs->blogSubscribe(mBlogId, true);
	}
	updateChannelMsgs();
}


void BlogsDialog::toggleSelection(const QModelIndex &index)
{
	QItemSelectionModel *selectionModel = treeView->selectionModel();
	if (index.child(0, 0).isValid())
		selectionModel->select(index, QItemSelectionModel::Toggle);
}

void BlogsDialog::showChannelDetails()
{
	if (mBlogId == "")
	{
	return;
	}

	if (!rsBlogs)
		return;

  //static ChannelDetails *channelui = new ChannelDetails();

	//channelui->showDetails(mBlogId);
	//channelui->show();

}
