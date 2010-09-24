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

#include <QMenu>
#include <QTimer>
#include <QStandardItemModel>

#include <iostream>
#include <algorithm>

#include <retroshare/rschannels.h>

#include "ChannelFeed.h"

#include "gui/feeds/ChanMsgItem.h"

#include "gui/channels/CreateChannel.h"
#include "gui/channels/ChannelDetails.h"
#include "gui/channels/CreateChannelMsg.h"
#include "gui/channels/EditChanDetails.h"
#include "gui/channels/ShareKey.h"

#include "gui/ChanGroupDelegate.h"

#define CHAN_DEFAULT_IMAGE ":/images/channels.png"

/****
 * #define CHAN_DEBUG
 ***/

/** Constructor */
ChannelFeed::ChannelFeed(QWidget *parent)
: RsAutoUpdatePage(1000,parent)
{
  	/* Invoke the Qt Designer generated object setup routine */
  	setupUi(this);

  	connect(actionCreate_Channel, SIGNAL(triggered()), this, SLOT(createChannel()));
  	connect(postButton, SIGNAL(clicked()), this, SLOT(createMsg()));
  	connect(subscribeButton, SIGNAL( clicked( void ) ), this, SLOT( subscribeChannel ( void ) ) );
  	connect(unsubscribeButton, SIGNAL( clicked( void ) ), this, SLOT( unsubscribeChannel ( void ) ) );

	/*************** Setup Left Hand Side (List of Channels) ****************/
	
    connect(treeView, SIGNAL(clicked(const QModelIndex &)), this, SLOT(selectChannel(const QModelIndex &)));
	connect(treeView, SIGNAL(activated(const QModelIndex &)), this, SLOT(toggleSelection(const QModelIndex &)));
	connect(treeView, SIGNAL(customContextMenuRequested( QPoint ) ), this, SLOT( channelListCustomPopupMenu( QPoint ) ) );

  	mChannelId = "";
	model = new QStandardItemModel(0, 3, this);
	model->setHeaderData(0, Qt::Horizontal, tr("Name"), Qt::DisplayRole);
	model->setHeaderData(1, Qt::Horizontal, tr("Popularity"), Qt::DisplayRole);
	model->setHeaderData(2, Qt::Horizontal, tr("ID"), Qt::DisplayRole);

	treeView->setModel(model);
	treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);

	treeView->setItemDelegate(new ChanGroupDelegate());
	treeView->setRootIsDecorated(true);

	// hide header and id column
	treeView->setHeaderHidden(true);
	treeView->hideColumn(2);
	
	/* Set header resize modes and initial section sizes TreeView*/
    QHeaderView * _header = treeView->header () ;
    _header->setResizeMode ( 1, QHeaderView::Custom);
    _header->resizeSection ( 0, 190 );
    _header->resizeSection ( 1, 22 );
    _header->resizeSection ( 2, 22 );
    
    // set ChannelList Font  
	itemFont = QFont("ARIAL", 10);
	itemFont.setBold(true);
	
	QStandardItem *ownChannels = new QStandardItem(tr("Own Channels"));
    ownChannels->setFont(itemFont);
    ownChannels->setForeground(QBrush(QColor(79, 79, 79)));

	QStandardItem *subcribedChannels = new QStandardItem(tr("Subscribed Channels"));
    subcribedChannels->setFont(itemFont);    
    subcribedChannels->setForeground(QBrush(QColor(79, 79, 79)));
    
	QStandardItem *popularChannels = new QStandardItem(tr("Popular Channels"));	
    popularChannels->setFont(itemFont);
    popularChannels->setForeground(QBrush(QColor(79, 79, 79)));
    
	QStandardItem *otherChannels = new QStandardItem(tr("Other Channels"));
    otherChannels->setFont(itemFont);
    otherChannels->setForeground(QBrush(QColor(79, 79, 79)));

	model->appendRow(ownChannels);
	model->appendRow(subcribedChannels);
	model->appendRow(popularChannels);
	model->appendRow(otherChannels);

    treeView->expand(ownChannels->index());
    treeView->expand(subcribedChannels->index());

	//added from ahead
	updateChannelList();
    
	mChannelFont = QFont("MS SANS SERIF", 22);	
	nameLabel->setFont(mChannelFont);    
	nameLabel->setMinimumWidth(20);
  
    // Setup Channel Menu:
    QMenu *channelmenu = new QMenu();
    channelmenu->addAction(actionCreate_Channel); 
    channelmenu->addSeparator();
    channelpushButton->setMenu(channelmenu);

    updateChannelMsgs();

}

void ChannelFeed::channelListCustomPopupMenu( QPoint point )
{
      ChannelInfo ci;
      if (!rsChannels->getChannelInfo(mChannelId, ci))
      {
            return;
      }

      QMenu contextMnu( this );
      
      postchannelAct = new QAction(QIcon(":/images/mail_reply.png"), tr( "Post to Channel" ), this );
      connect( postchannelAct , SIGNAL( triggered() ), this, SLOT( createMsg() ) );
      
      subscribechannelAct = new QAction(QIcon(":/images/edit_add24.png"), tr( "Subscribe to Channel" ), this );
      connect( subscribechannelAct , SIGNAL( triggered() ), this, SLOT( subscribeChannel() ) );

      unsubscribechannelAct = new QAction(QIcon(":/images/cancel.png"), tr( "Unsubscribe to Channel" ), this );
      connect( unsubscribechannelAct , SIGNAL( triggered() ), this, SLOT( unsubscribeChannel() ) );
      
      channeldetailsAct = new QAction(QIcon(":/images/info16.png"), tr( "Show Channel Details" ), this );
      connect( channeldetailsAct , SIGNAL( triggered() ), this, SLOT( showChannelDetails() ) );

      restoreKeysAct = new QAction(QIcon(":/images/settings16.png"), tr("Restore Publish Rights for Channel" ), this );
      connect( restoreKeysAct , SIGNAL( triggered() ), this, SLOT( restoreChannelKeys() ) );

      editChannelDetailAct = new QAction(QIcon(":/images/edit_16.png"), tr("Edit Channel Details"), this);
      connect( editChannelDetailAct, SIGNAL( triggered() ), this, SLOT( editChannelDetail() ) );

      shareKeyAct = new QAction(QIcon(":/images/gpgp_key_generate.png"), tr("Share Channel"), this);
      connect( shareKeyAct, SIGNAL( triggered() ), this, SLOT( shareKey() ) );

      if((ci.channelFlags & RS_DISTRIB_PUBLISH) && (ci.channelFlags & RS_DISTRIB_ADMIN))
      {
        contextMnu.addAction( postchannelAct );
        contextMnu.addSeparator();
        contextMnu.addAction( editChannelDetailAct);
        contextMnu.addAction( shareKeyAct );
        contextMnu.addAction( channeldetailsAct );
      }
      else if (ci.channelFlags & RS_DISTRIB_PUBLISH)
      {
        contextMnu.addAction( postchannelAct );
        contextMnu.addSeparator();
        contextMnu.addAction( channeldetailsAct );
        contextMnu.addAction( shareKeyAct );
      }
      else if (ci.channelFlags & RS_DISTRIB_SUBSCRIBED)
      {
        contextMnu.addAction( unsubscribechannelAct );
        contextMnu.addSeparator();
        contextMnu.addAction( channeldetailsAct );
    	contextMnu.addAction( restoreKeysAct );
      }
      else
      {      
        contextMnu.addAction( subscribechannelAct );
        contextMnu.addSeparator();
        contextMnu.addAction( channeldetailsAct );
        contextMnu.addAction( restoreKeysAct );
      }

      contextMnu.exec(QCursor::pos());
}

void ChannelFeed::createChannel()
{
	CreateChannel cf (this);
	cf.exec();
}

void ChannelFeed::channelSelection()
{
	/* which item was selected? */


	/* update mChannelId */

	updateChannelMsgs();
}



/*************************************************************************************/
/*************************************************************************************/
/*************************************************************************************/

void ChannelFeed::deleteFeedItem(QWidget *item, uint32_t type)
{
	return;
}

void ChannelFeed::openChat(std::string peerId)
{
	return;
}

void ChannelFeed::editChannelDetail(){
    
    EditChanDetails editUi(this, 0, mChannelId);
    editUi.exec();

    return;
}

void ChannelFeed::shareKey()
{
    ShareKey shareUi(this, 0, mChannelId);
    shareUi.exec();

    return;
}

void ChannelFeed::createMsg()
{
	if (mChannelId == "")
	{
	return;
	}

	CreateChannelMsg *msgDialog = new CreateChannelMsg(mChannelId);

	msgDialog->show();

	/* window will destroy itself! */
}

void ChannelFeed::selectChannel( std::string cId)
{
	mChannelId = cId;

	updateChannelMsgs();
}

void ChannelFeed::restoreChannelKeys(){

	rsChannels->channelRestoreKeys(mChannelId);
}

void ChannelFeed::selectChannel(const QModelIndex &index)
{
	int row = index.row();
	int col = index.column();
	if (col != 2) {
		QModelIndex sibling = index.sibling(row, 2);
		if (sibling.isValid())
			mChannelId = sibling.data().toString().toStdString();
	} else
		mChannelId = index.data().toString().toStdString();
	updateChannelMsgs();
}

void ChannelFeed::updateDisplay()
{
	std::list<std::string> chanIds;
	std::list<std::string>::iterator it;
	if (!rsChannels)
		return;

	if (rsChannels->channelsChanged(chanIds))
	{
		/* update Forums List */
		updateChannelList();

		it = std::find(chanIds.begin(), chanIds.end(), mChannelId);
		if (it != chanIds.end())
		{
			updateChannelMsgs();
		}
	}
}

void ChannelFeed::updateChannelList()
{

	std::list<ChannelInfo> channelList;
	std::list<ChannelInfo>::iterator it;
	if (!rsChannels)
	{
		return;
	}

	rsChannels->getChannelList(channelList);

	/* get the ids for our lists */
	std::list<std::string> adminIds;
	std::list<std::string> subIds;
	std::list<std::string> popIds;
	std::list<std::string> otherIds;
	std::multimap<uint32_t, std::string> popMap;

	for(it = channelList.begin(); it != channelList.end(); it++)
	{
		/* sort it into Publish (Own), Subscribed, Popular and Other */
		uint32_t flags = it->channelFlags;

		if (flags & RS_DISTRIB_ADMIN)
		{
			adminIds.push_back(it->channelId);
		}
		else if (flags & RS_DISTRIB_SUBSCRIBED)
		{
			subIds.push_back(it->channelId);
		}
		else
		{
			/* rate the others by popularity */
			popMap.insert(std::make_pair(it->pop, it->channelId));
		}
	}

	/* iterate backwards through popMap - take the top 5 or 10% of list */
	uint32_t popCount = 5;
	if (popCount < popMap.size() / 10)
	{
		popCount = popMap.size() / 10;
	}

	uint32_t i = 0;
	std::multimap<uint32_t, std::string>::reverse_iterator rit;
	for(rit = popMap.rbegin(); rit != popMap.rend(); rit++)
	{
		if(i < popCount){
			popIds.push_back(rit->second);
			i++;
		}else{
			otherIds.push_back(rit->second);
		}
	}

	/* now we have our lists ---> update entries */

	updateChannelListOwn(adminIds);
	updateChannelListSub(subIds);
	updateChannelListPop(popIds);
	updateChannelListOther(otherIds);
}

void ChannelFeed::updateChannelListOwn(std::list<std::string> &ids)
{
    std::list<std::string>::iterator iit;

    /* remove rows with groups before adding new ones */
    model->item(OWN)->removeRows(0, model->item(OWN)->rowCount());

    for (iit = ids.begin(); iit != ids.end(); iit ++) {
#ifdef CHAN_DEBUG
		std::cerr << "ChannelFeed::updateChannelListOwn(): " << *iit << std::endl;
#endif
		QStandardItem *ownGroup = model->item(OWN);
		QList<QStandardItem *> channel;
		QStandardItem *chNameItem = new QStandardItem();
		QStandardItem *chPopItem = new QStandardItem();
		QStandardItem *chIdItem = new QStandardItem();
		chNameItem->setSizeHint( QSize( 22,22 ) ); 

		ChannelInfo ci;
		if (rsChannels && rsChannels->getChannelInfo(*iit, ci)) {
			chNameItem->setData(QVariant(QString::fromStdWString(ci.channelName)), Qt::DisplayRole);
			//chPopItem->setData(QVariant(QString::number(ci.pop)), Qt::DisplayRole);
			chIdItem->setData(QVariant(QString::fromStdString(ci.channelId)), Qt::DisplayRole);
			chNameItem->setToolTip(tr("Popularity: %1\nFetches: %2\nAvailable: %3").arg(QString::number(ci.pop)).arg(9999).arg(9999));
			
			  if(ci.pngImageLen != 0){

        QPixmap chanImage;
        chanImage.loadFromData(ci.pngChanImage, ci.pngImageLen, "PNG");
        chNameItem->setData(QIcon(chanImage), Qt::DecorationRole);
        }else{
        QPixmap defaulImage(CHAN_DEFAULT_IMAGE);
        chNameItem->setData(QIcon(defaulImage), Qt::DecorationRole);
        }
					
        int popcount = ci.pop;		
        /* set Popularity icon  */
        if (popcount == 0) 
        {
            chPopItem->setData(QIcon(QString::fromUtf8(":/images/hot_0.png")), Qt::DecorationRole);
        } 
        else if (popcount < 2) 
        {
            chPopItem->setData(QIcon(QString::fromUtf8(":/images/hot_1.png")), Qt::DecorationRole);
        } 
        else if (popcount < 4) 
        {
            chPopItem->setData(QIcon(QString::fromUtf8(":/images/hot_2.png")), Qt::DecorationRole);
        } 
        else if (popcount < 8) 
        {
            chPopItem->setData(QIcon(QString::fromUtf8(":/images/hot_3.png")), Qt::DecorationRole);
        } 
        else if (popcount < 16) {
            chPopItem->setData(QIcon(QString::fromUtf8(":/images/hot_4.png")), Qt::DecorationRole);
        } 
        else
        {
            chPopItem->setData(QIcon(QString::fromUtf8(":/images/hot_5.png")), Qt::DecorationRole);
        } 		
					
					
		} else {
                        chNameItem->setData(QVariant(QString(tr("Unknown Channel"))), Qt::DisplayRole);
			chPopItem->setData(QVariant(QString::fromStdString(*iit)), Qt::DisplayRole);
                        chNameItem->setToolTip(tr("Unknown Channel\nNo Description"));
		}
    
		channel.append(chNameItem);
		channel.append(chPopItem);
		channel.append(chIdItem);
		ownGroup->appendRow(channel);
	}
}

void ChannelFeed::updateChannelListSub(std::list<std::string> &ids)
{
    std::list<std::string>::iterator iit;

    /* remove rows with groups before adding new ones */
    model->item(SUBSCRIBED)->removeRows(0, model->item(SUBSCRIBED)->rowCount());

    for (iit = ids.begin(); iit != ids.end(); iit ++) {
#ifdef CHAN_DEBUG
		std::cerr << "ChannelFeed::updateChannelListSub(): " << *iit << std::endl;
#endif
		QStandardItem *ownGroup = model->item(SUBSCRIBED);
		QList<QStandardItem *> channel;
		QStandardItem *chNameItem = new QStandardItem();
		QStandardItem *chPopItem = new QStandardItem();
		QStandardItem *chIdItem = new QStandardItem();
		chNameItem->setSizeHint( QSize( 22,22 ) ); 

		ChannelInfo ci;
		if (rsChannels && rsChannels->getChannelInfo(*iit, ci)) {
			chNameItem->setData(QVariant(QString::fromStdWString(ci.channelName)), Qt::DisplayRole);
			chIdItem->setData(QVariant(QString::fromStdString(ci.channelId)), Qt::DisplayRole);
			chNameItem->setToolTip(tr("Popularity: %1\nFetches: %2\nAvailable: %3").arg(QString::number(ci.pop)).arg(9999).arg(9999));
			
          if(ci.pngImageLen != 0){

          QPixmap chanImage;
          chanImage.loadFromData(ci.pngChanImage, ci.pngImageLen, "PNG");
          chNameItem->setData(QIcon(chanImage), Qt::DecorationRole);
          }else{
          QPixmap defaulImage(CHAN_DEFAULT_IMAGE);
          chNameItem->setData(QIcon(defaulImage), Qt::DecorationRole);
          }

		      /* set Popularity icon  */
		      int popcount = ci.pop;

		        if (popcount == 0)
		        {
		            chPopItem->setData(QIcon(QString::fromUtf8(":/images/hot_0.png")), Qt::DecorationRole);
		        }
		        else if (popcount < 2)
		        {
		            chPopItem->setData(QIcon(QString::fromUtf8(":/images/hot_1.png")), Qt::DecorationRole);
		        }
		        else if (popcount < 4)
		        {
		            chPopItem->setData(QIcon(QString::fromUtf8(":/images/hot_2.png")), Qt::DecorationRole);
		        }
		        else if (popcount < 8)
		        {
		            chPopItem->setData(QIcon(QString::fromUtf8(":/images/hot_3.png")), Qt::DecorationRole);
		        }
		        else if (popcount < 16) 
		        {
		            chPopItem->setData(QIcon(QString::fromUtf8(":/images/hot_4.png")), Qt::DecorationRole);
		        }
		        else
		        {
		            chPopItem->setData(QIcon(QString::fromUtf8(":/images/hot_5.png")), Qt::DecorationRole);
		        }


				} else {
                                        chNameItem->setData(QVariant(QString(tr("Unknown Channel"))), Qt::DisplayRole);
					chPopItem->setData(QVariant(QString::fromStdString(*iit)), Qt::DisplayRole);
                                        chNameItem->setToolTip(tr("Unknown Channel\nNo Description"));
				}

		channel.append(chNameItem);
		channel.append(chPopItem);
		channel.append(chIdItem);
		ownGroup->appendRow(channel);
	}

}

void ChannelFeed::updateChannelListPop(std::list<std::string> &ids)
{
    std::list<std::string>::iterator iit;

    /* remove rows with groups before adding new ones */
    model->item(POPULAR)->removeRows(0, model->item(POPULAR)->rowCount());

    for (iit = ids.begin(); iit != ids.end(); iit ++) {
#ifdef CHAN_DEBUG
		std::cerr << "ChannelFeed::updateChannelListPop(): " << *iit << std::endl;
#endif
		QStandardItem *ownGroup = model->item(POPULAR);
		QList<QStandardItem *> channel;
		QStandardItem *chNameItem = new QStandardItem();
		QStandardItem *chPopItem = new QStandardItem();
		QStandardItem *chIdItem = new QStandardItem();
		chNameItem->setSizeHint( QSize( 22,22 ) ); 

		ChannelInfo ci;
		if (rsChannels && rsChannels->getChannelInfo(*iit, ci)) {
			chNameItem->setData(QVariant(QString::fromStdWString(ci.channelName)), Qt::DisplayRole);
			chIdItem->setData(QVariant(QString::fromStdString(ci.channelId)), Qt::DisplayRole);
			chNameItem->setToolTip(tr("Popularity: %1\nFetches: %2\nAvailable: %3").arg(QString::number(ci.pop)).arg(9999).arg(9999));
			
			  if(ci.pngImageLen != 0){

        QPixmap chanImage;
        chanImage.loadFromData(ci.pngChanImage, ci.pngImageLen, "PNG");
        chNameItem->setData(QIcon(chanImage), Qt::DecorationRole);
        }else{
        QPixmap defaulImage(CHAN_DEFAULT_IMAGE);
        chNameItem->setData(QIcon(defaulImage), Qt::DecorationRole);
        }

	      /* set Popularity icon  */
	      int popcount = ci.pop;

	        if (popcount == 0)
	        {
	            chPopItem->setData(QIcon(QString::fromUtf8(":/images/hot_0.png")), Qt::DecorationRole);
	        }
	        else if (popcount < 2)
	        {
	            chPopItem->setData(QIcon(QString::fromUtf8(":/images/hot_1.png")), Qt::DecorationRole);
	        }
	        else if (popcount < 4)
	        {
	            chPopItem->setData(QIcon(QString::fromUtf8(":/images/hot_2.png")), Qt::DecorationRole);
	        }
	        else if (popcount < 8)
	        {
	            chPopItem->setData(QIcon(QString::fromUtf8(":/images/hot_3.png")), Qt::DecorationRole);
	        }
	        else if (popcount < 16) {
	            chPopItem->setData(QIcon(QString::fromUtf8(":/images/hot_4.png")), Qt::DecorationRole);
	        }
	        else
	        {
	            chPopItem->setData(QIcon(QString::fromUtf8(":/images/hot_5.png")), Qt::DecorationRole);
	        }


			} else {
                                chNameItem->setData(QVariant(QString(tr("Unknown Channel"))), Qt::DisplayRole);
				chPopItem->setData(QVariant(QString::fromStdString(*iit)), Qt::DisplayRole);
                                chNameItem->setToolTip(tr("Unknown Channel\nNo Description"));
			}

		channel.append(chNameItem);
		channel.append(chPopItem);
		channel.append(chIdItem);
		ownGroup->appendRow(channel);
	}
}

void ChannelFeed::updateChannelListOther(std::list<std::string> &ids)
{
    std::list<std::string>::iterator iit;

    /* remove rows with groups before adding new ones */
    model->item(OTHER)->removeRows(0, model->item(OTHER)->rowCount());

    for (iit = ids.begin(); iit != ids.end(); iit ++) {
#ifdef CHAN_DEBUG
		std::cerr << "ChannelFeed::updateChannelListOther(): " << *iit << std::endl;
#endif
		QStandardItem *ownGroup = model->item(OTHER);
		QList<QStandardItem *> channel;
		QStandardItem *chNameItem = new QStandardItem();
		QStandardItem *chPopItem = new QStandardItem();
		QStandardItem *chIdItem = new QStandardItem();
		chNameItem->setSizeHint( QSize( 22,22 ) );

		ChannelInfo ci;
		if (rsChannels && rsChannels->getChannelInfo(*iit, ci)) {
			chNameItem->setData(QVariant(QString::fromStdWString(ci.channelName)), Qt::DisplayRole);
			chIdItem->setData(QVariant(QString::fromStdString(ci.channelId)), Qt::DisplayRole);
			chNameItem->setToolTip(tr("Popularity: %1\nFetches: %2\nAvailable: %3"
					).arg(QString::number(ci.pop)).arg(9999).arg(9999));

			  if(ci.pngImageLen != 0){

      QPixmap chanImage;
      chanImage.loadFromData(ci.pngChanImage, ci.pngImageLen, "PNG");
      chNameItem->setData(QIcon(chanImage), Qt::DecorationRole);
      }else{
      QPixmap defaulImage(CHAN_DEFAULT_IMAGE);
      chNameItem->setData(QIcon(defaulImage), Qt::DecorationRole);
      }

		      /* set Popularity icon  */
		      int popcount = ci.pop;

		        if (popcount == 0)
		        {
		            chPopItem->setData(QIcon(QString::fromUtf8(":/images/hot_0.png")), Qt::DecorationRole);
		        }
		        else if (popcount < 2)
		        {
		            chPopItem->setData(QIcon(QString::fromUtf8(":/images/hot_1.png")), Qt::DecorationRole);
		        }
		        else if (popcount < 4)
		        {
		            chPopItem->setData(QIcon(QString::fromUtf8(":/images/hot_2.png")), Qt::DecorationRole);
		        }
		        else if (popcount < 8)
		        {
		            chPopItem->setData(QIcon(QString::fromUtf8(":/images/hot_3.png")), Qt::DecorationRole);
		        }
		        else if (popcount < 16) {
		            chPopItem->setData(QIcon(QString::fromUtf8(":/images/hot_4.png")), Qt::DecorationRole);
		        }
		        else
		        {
		            chPopItem->setData(QIcon(QString::fromUtf8(":/images/hot_5.png")), Qt::DecorationRole);
		        }


				} else {
                                        chNameItem->setData(QVariant(QString(tr("Unknown Channel"))), Qt::DisplayRole);
					chPopItem->setData(QVariant(QString::fromStdString(*iit)), Qt::DisplayRole);
                                        chNameItem->setToolTip(tr("Unknown Channel\nNo Description"));
				}

		channel.append(chNameItem);
		channel.append(chPopItem);
		channel.append(chIdItem);
		ownGroup->appendRow(channel);
	}
}

void ChannelFeed::updateChannelMsgs()
{
	if (!rsChannels)
		return;

	ChannelInfo ci;
	if (!rsChannels->getChannelInfo(mChannelId, ci))
	{
		postButton->setEnabled(false);
		subscribeButton->setEnabled(false);
		unsubscribeButton->setEnabled(false);
                nameLabel->setText(tr("No Channel Selected"));
		iconLabel->setPixmap(QPixmap(":/images/channels.png"));
		iconLabel->setEnabled(false);
		return;
	}
	
	if(ci.pngImageLen != 0){

		QPixmap chanImage;
		chanImage.loadFromData(ci.pngChanImage, ci.pngImageLen, "PNG");
		iconLabel->setPixmap(chanImage);
		iconLabel->setStyleSheet("QLabel{border: 3px solid white;}");
	}else{
		QPixmap defaulImage(CHAN_DEFAULT_IMAGE);
		iconLabel->setPixmap(defaulImage);
		iconLabel->setStyleSheet("QLabel{border: 2px solid white;border-radius: 10px;}");
	}
	iconLabel->setEnabled(true);

	/* set textcolor for Channel name  */
	QString channelStr("<span style=\"font-size:22pt; font-weight:500;"
                               "color:#4F4F4F;\">%1</span>");
	
	/* set Channel name */
	QString cname = QString::fromStdWString(ci.channelName);
  nameLabel->setText(channelStr.arg(cname));

	/* do buttons */
	if (ci.channelFlags & RS_DISTRIB_SUBSCRIBED)
	{
		subscribeButton->setEnabled(false);
		unsubscribeButton->setEnabled(true);
	}
	else
	{
		subscribeButton->setEnabled(true);
		unsubscribeButton->setEnabled(false);
	}

	if (ci.channelFlags & RS_DISTRIB_PUBLISH)
	{
		postButton->setEnabled(true);
	}
	else
	{
		postButton->setEnabled(false);
	}

	/* replace all the messages with new ones */
	std::list<ChanMsgItem *>::iterator mit;
	for(mit = mChanMsgItems.begin(); mit != mChanMsgItems.end(); mit++)
	{
		delete (*mit);
	}
	mChanMsgItems.clear();

	std::list<ChannelMsgSummary> msgs;
	std::list<ChannelMsgSummary>::iterator it;

	rsChannels->getChannelMsgList(mChannelId, msgs);

	for(it = msgs.begin(); it != msgs.end(); it++)
	{
		ChanMsgItem *cmi = new ChanMsgItem(this, 0, mChannelId, it->msgId, true);

		mChanMsgItems.push_back(cmi);
		verticalLayout_2->addWidget(cmi);
	}
}


void ChannelFeed::unsubscribeChannel()
{
#ifdef CHAN_DEBUG
	std::cerr << "ChannelFeed::unsubscribeChannel()";
	std::cerr << std::endl;
#endif
	if (rsChannels)
	{
		rsChannels->channelSubscribe(mChannelId, false);
	}
	updateChannelMsgs();
}


void ChannelFeed::subscribeChannel()
{
#ifdef CHAN_DEBUG
	std::cerr << "ChannelFeed::subscribeChannel()";
	std::cerr << std::endl;
#endif
	if (rsChannels)
	{
		rsChannels->channelSubscribe(mChannelId, true);
	}
	updateChannelMsgs();
}


void ChannelFeed::toggleSelection(const QModelIndex &index)
{
	QItemSelectionModel *selectionModel = treeView->selectionModel();
	if (index.child(0, 0).isValid())
		selectionModel->select(index, QItemSelectionModel::Toggle);
}

void ChannelFeed::showChannelDetails()
{
	if (mChannelId == "")
	{
	return;
	}

	if (!rsChannels)
		return;

	ChannelDetails channelui (this);

	channelui.showDetails(mChannelId);
	channelui.exec();
}
