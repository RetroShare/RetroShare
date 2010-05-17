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
#include <QPoint>

#include <iostream>
#include <algorithm>

#include "rsiface/rsblogs.h"
#include "rsiface/rspeers.h" //to retrieve peer/usrId info

#include "BlogsDialog.h"

#include "BlogsMsgItem.h"
#include "CreateBlog.h"
#include "CreateBlogMsg.h"
#include "BlogDetails.h"

#include "gui/ChanGroupDelegate.h"
#include "gui/GeneralMsgDialog.h"

#define BLOG_DEFAULT_IMAGE ":/images/hi64-app-kblogger.png"


/** Constructor */
BlogsDialog::BlogsDialog(QWidget *parent)
: MainPage (parent)
{
  	/* Invoke the Qt Designer generated object setup routine */
  	setupUi(this);

  	connect(actionCreate_Blog, SIGNAL(triggered()), this, SLOT(createBlog()));
  	connect(postButton, SIGNAL(clicked()), this, SLOT(createMsg()));
  	connect(subscribeButton, SIGNAL( clicked( void ) ), this, SLOT( subscribeBlog ( void ) ) );
  	connect(unsubscribeButton, SIGNAL( clicked( void ) ), this, SLOT( unsubscribeBlog ( void ) ) );

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

    connect(treeView, SIGNAL(clicked(const QModelIndex &)), this, SLOT(selectBlog(const QModelIndex &)));
    connect(treeView, SIGNAL(activated(const QModelIndex &)), this, SLOT(toggleSelection(const QModelIndex &)));
    connect(treeView, SIGNAL(customContextMenuRequested( QPoint ) ), this, SLOT( blogListCustomPopupMenu( QPoint ) ) );

    //added from ahead
    updateBlogList();

    mBlogFont = QFont("MS SANS SERIF", 22);
    nameLabel->setFont(mBlogFont);
    
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
  
    QMenu *blogmenu = new QMenu();
    blogmenu->addAction(actionCreate_Blog); 
    blogmenu->addSeparator();
    blogpushButton->setMenu(blogmenu);

	
    QTimer *timer = new QTimer(this);
    timer->connect(timer, SIGNAL(timeout()), this, SLOT(checkUpdate()));
    timer->start(1000);
}

void BlogsDialog::blogListCustomPopupMenu( QPoint point )
{
      BlogInfo bi;
      if (!rsBlogs->getBlogInfo(mBlogId, bi))
      {
      return;
      }

      QMenu contextMnu( this );
      
      QAction *createblogpostAct = new QAction(QIcon(":/images/mail_reply.png"), tr( "Post to Blog" ), this );
      connect( createblogpostAct , SIGNAL( triggered() ), this, SLOT( createMsg() ) );
      
      QAction *subscribeblogAct = new QAction(QIcon(":/images/edit_add24.png"), tr( "Subscribe to Blog" ), this );
      connect( subscribeblogAct , SIGNAL( triggered() ), this, SLOT( subscribeBlog() ) );

      QAction *unsubscribeblogAct = new QAction(QIcon(":/images/cancel.png"), tr( "Unsubscribe to Blog" ), this );
      connect( unsubscribeblogAct , SIGNAL( triggered() ), this, SLOT( unsubscribeBlog() ) );
      
      QAction *blogdetailsAct = new QAction(QIcon(":/images/info16.png"), tr( "Show Blog Details" ), this );
      connect( blogdetailsAct , SIGNAL( triggered() ), this, SLOT( showBlogDetails() ) );
      
      contextMnu.clear();

      if (bi.blogFlags & RS_DISTRIB_PUBLISH)
      {
        contextMnu.addAction( createblogpostAct );
        contextMnu.addSeparator();
        contextMnu.addAction( blogdetailsAct );
      }
      else if (bi.blogFlags & RS_DISTRIB_SUBSCRIBED)
      {
        contextMnu.addAction( unsubscribeblogAct );
        contextMnu.addSeparator();
        contextMnu.addAction( blogdetailsAct );;
      }
      else
      {      
        contextMnu.addAction( subscribeblogAct );
        contextMnu.addSeparator();
        contextMnu.addAction( blogdetailsAct );
      }

      contextMnu.exec(QCursor::pos());
}

void BlogsDialog::createBlog()
{
	CreateBlog cf (this, false);

	cf.setWindowTitle(tr("Create a new Blog"));
	cf.exec();
}

void BlogsDialog::blogSelection()
{
	/* which item was selected? */


	/* update mBlogId */

	updateBlogMsgs();
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
#ifdef BLOG_DEBUG
	std::cerr << "BlogsDialog::openMsg()";
	std::cerr << std::endl;
#endif
	GeneralMsgDialog *msgDialog = new GeneralMsgDialog(NULL);


	msgDialog->addDestination(type, grpId, inReplyTo);

	msgDialog->show();

	/* window will destroy itself! */
}

void BlogsDialog::createMsg()
{
	if (mBlogId == "")
	{
	return;
	}

	CreateBlogMsg *msgDialog = new CreateBlogMsg(mBlogId);

	msgDialog->show();

	/* window will destroy itself! */
}

void BlogsDialog::selectBlog( std::string bId)
{
	mBlogId = bId;

	updateBlogMsgs();
}

void BlogsDialog::selectBlog(const QModelIndex &index)
{
	int row = index.row();
	int col = index.column();
	if (col != 1) {
		QModelIndex sibling = index.sibling(row, 1);
		if (sibling.isValid())
			mBlogId = sibling.data().toString().toStdString();
	} else
		mBlogId = index.data().toString().toStdString();
	updateBlogMsgs();
}

void BlogsDialog::checkUpdate()
{
	std::list<std::string> blogIds;
	std::list<std::string>::iterator it;
	if (!rsBlogs)
		return;

	if (rsBlogs->blogsChanged(blogIds))
	{
		/* update Blogs List */
		updateBlogList();

		it = std::find(blogIds.begin(), blogIds.end(), mBlogId);
		if (it != blogIds.end())
		{
			updateBlogMsgs();
		}
	}
}

void BlogsDialog::updateBlogList()
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

	updateBlogListOwn(adminIds);
	updateBlogListSub(subIds);
	updateBlogListPop(popIds);
	updateBlogListOther(otherIds);
}

void BlogsDialog::updateBlogListOwn(std::list<std::string> &ids)
{
	std::list<std::string>::iterator iit;

	/* remove rows with groups before adding new ones */
	model->item(OWN)->removeRows(0, model->item(OWN)->rowCount());

	for (iit = ids.begin(); iit != ids.end(); iit ++) {
#ifdef BLOG_DEBUG
		std::cerr << "BlogsDialog::updateBlogListOwn(): " << *iit << std::endl;
#endif
		QStandardItem *ownGroup = model->item(OWN);
		QList<QStandardItem *> channel;
		QStandardItem *item1 = new QStandardItem();
		QStandardItem *item2 = new QStandardItem();

		BlogInfo bi;
		if (rsBlogs && rsBlogs->getBlogInfo(*iit, bi)) {
			item1->setData(QVariant(QString::fromStdWString(bi.blogName)), Qt::DisplayRole);
			item2->setData(QVariant(QString::fromStdString(bi.blogId)), Qt::DisplayRole);
			item1->setToolTip(tr("Popularity: %1\nFetches: %2\nAvailable: %3"
					).arg(QString::number(bi.pop)).arg(9999).arg(9999));
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

void BlogsDialog::updateBlogListSub(std::list<std::string> &ids)
{
	std::list<std::string>::iterator iit;

	/* remove rows with groups before adding new ones */
	model->item(SUBSCRIBED)->removeRows(0, model->item(SUBSCRIBED)->rowCount());

	for (iit = ids.begin(); iit != ids.end(); iit ++) {
#ifdef BLOG_DEBUG
		std::cerr << "BlogsDialog::updateBlogListSub(): " << *iit << std::endl;
#endif
		QStandardItem *ownGroup = model->item(SUBSCRIBED);
		QList<QStandardItem *> channel;
		QStandardItem *item1 = new QStandardItem();
		QStandardItem *item2 = new QStandardItem();

		BlogInfo bi;
		if (rsBlogs && rsBlogs->getBlogInfo(*iit, bi)) {
			item1->setData(QVariant(QString::fromStdWString(bi.blogName)), Qt::DisplayRole);
			item2->setData(QVariant(QString::fromStdString(bi.blogId)), Qt::DisplayRole);
			item1->setToolTip(tr("Popularity: %1\nFetches: %2\nAvailable: %3"
					).arg(QString::number(bi.pop)).arg(9999).arg(9999));
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

void BlogsDialog::updateBlogListPop(std::list<std::string> &ids)
{
	std::list<std::string>::iterator iit;

	/* remove rows with groups before adding new ones */
	model->item(POPULAR)->removeRows(0, model->item(POPULAR)->rowCount());

	for (iit = ids.begin(); iit != ids.end(); iit ++) {
#ifdef BLOG_DEBUG
		std::cerr << "BlogsDialog::updateBlogListPop(): " << *iit << std::endl;
#endif
		QStandardItem *ownGroup = model->item(POPULAR);
		QList<QStandardItem *> channel;
		QStandardItem *item1 = new QStandardItem();
		QStandardItem *item2 = new QStandardItem();

		BlogInfo bi;
		if (rsBlogs && rsBlogs->getBlogInfo(*iit, bi)) {
			item1->setData(QVariant(QString::fromStdWString(bi.blogName)), Qt::DisplayRole);
			item2->setData(QVariant(QString::fromStdString(bi.blogId)), Qt::DisplayRole);
			item1->setToolTip(tr("Popularity: %1\nFetches: %2\nAvailable: %3"
					).arg(QString::number(bi.pop)).arg(9999).arg(9999));
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

void BlogsDialog::updateBlogListOther(std::list<std::string> &ids)
{
	std::list<std::string>::iterator iit;

	/* remove rows with groups before adding new ones */
	model->item(OTHER)->removeRows(0, model->item(OTHER)->rowCount());

	for (iit = ids.begin(); iit != ids.end(); iit ++) {
#ifdef BLOG_DEBUG
		std::cerr << "BlogsDialog::updateBlogListOther(): " << *iit << std::endl;
#endif
		QStandardItem *ownGroup = model->item(OTHER);
		QList<QStandardItem *> channel;
		QStandardItem *item1 = new QStandardItem();
		QStandardItem *item2 = new QStandardItem();

		BlogInfo bi;
		if (rsBlogs && rsBlogs->getBlogInfo(*iit, bi)) {
			item1->setData(QVariant(QString::fromStdWString(bi.blogName)), Qt::DisplayRole);
			item2->setData(QVariant(QString::fromStdString(bi.blogId)), Qt::DisplayRole);
			item1->setToolTip(tr("Popularity: %1\nFetches: %2\nAvailable: %3"
					).arg(QString::number(bi.pop)).arg(9999).arg(9999));
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

void BlogsDialog::updateBlogMsgs()
{
	if (!rsBlogs)
		return;

	BlogInfo bi;
	if (!rsBlogs->getBlogInfo(mBlogId, bi))
	{
		postButton->setEnabled(false);
		subscribeButton->setEnabled(false);
		unsubscribeButton->setEnabled(false);
		nameLabel->setText("No Blog Selected");
		iconLabel->setEnabled(false);
		return;
	}
	
	if(bi.pngImageLen != 0){

		QPixmap blogImage;
		blogImage.loadFromData(bi.pngChanImage, bi.pngImageLen, "PNG");
		iconLabel->setPixmap(blogImage);
	}else{
		QPixmap defaultImage(BLOG_DEFAULT_IMAGE);
		iconLabel->setPixmap(defaultImage);
	}
	
	iconLabel->setEnabled(true);
	
	/* set textcolor for Blog name  */
	QString blogStr("<span style=\"font-size:22pt; font-weight:500;"
                               "color:white;\">%1</span>");
	
	/* set Blog name */
	QString bname = QString::fromStdWString(bi.blogName);
  nameLabel->setText(blogStr.arg(bname));

	/* do buttons */
	if (bi.blogFlags & RS_DISTRIB_SUBSCRIBED)
	{
		subscribeButton->setEnabled(false);
		unsubscribeButton->setEnabled(true);
		frame->setStyleSheet("QFrame#frame{ border: 2px solid #6E777F;border-radius: 10px;background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1,stop:0 #515D66, stop:1 #75818A); }");
		
	}
	else
	{
		subscribeButton->setEnabled(true);
		unsubscribeButton->setEnabled(false);
    frame->setStyleSheet("QFrame#frame{ border: 2px solid #6ACEFF;border-radius: 10px;background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1,stop:0 #0076B1, stop:1 #12A3EB); }");
	}

	if (bi.blogFlags & RS_DISTRIB_PUBLISH)
	{
		postButton->setEnabled(true);
		frame->setStyleSheet("QFrame#frame{ border: 2px solid #6ACEFF;border-radius: 10px;background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1,stop:0 #0076B1, stop:1 #12A3EB); }");
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

void BlogsDialog::unsubscribeBlog()
{
#ifdef BLOG_DEBUG
	std::cerr << "BlogsDialog::unsubscribeBlog()";
	std::cerr << std::endl;
#endif
	if (rsBlogs)
	{
		rsBlogs->blogSubscribe(mBlogId, false);
	}
	updateBlogMsgs();
}

void BlogsDialog::subscribeBlog()
{
#ifdef BLOG_DEBUG
	std::cerr << "BlogsDialog::subscribeBlog()";
	std::cerr << std::endl;
#endif
	if (rsBlogs)
	{
		rsBlogs->blogSubscribe(mBlogId, true);
	}
	updateBlogMsgs();
}

void BlogsDialog::toggleSelection(const QModelIndex &index)
{
	QItemSelectionModel *selectionModel = treeView->selectionModel();
	if (index.child(0, 0).isValid())
		selectionModel->select(index, QItemSelectionModel::Toggle);
}

void BlogsDialog::showBlogDetails()
{
	if (mBlogId == "")
	{
	return;
	}

	if (!rsBlogs)
		return;

	BlogDetails *blogui = new BlogDetails();

	blogui->showDetails(mBlogId);
	blogui->show();

	/* window will destroy itself! */
}
