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

#include "ChannelFeed.h"

#include "feeds/ChanMsgItem.h"

#include "channels/CreateChannel.h"
#include "channels/ChannelDetails.h"
#include "channels/CreateChannelMsg.h"
#include "channels/EditChanDetails.h"
#include "channels/ShareKey.h"
#include "notifyqt.h"

#include "ChanGroupDelegate.h"

#define CHAN_DEFAULT_IMAGE ":/images/channels.png"

#define COLUMN_NAME        0
#define COLUMN_POPULARITY  1
#define COLUMN_COUNT       2
#define COLUMN_DATA        COLUMN_NAME

#define ROLE_ID            Qt::UserRole
#define ROLE_CHANNEL_TITLE Qt::UserRole + 1

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
    connect(setAllAsReadButton, SIGNAL(clicked()), this, SLOT(setAllAsReadClicked()));

    connect(NotifyQt::getInstance(), SIGNAL(channelMsgReadSatusChanged(QString,QString,int)), this, SLOT(channelMsgReadSatusChanged(QString,QString,int)));

    /*************** Setup Left Hand Side (List of Channels) ****************/

    connect(treeView, SIGNAL(customContextMenuRequested( QPoint ) ), this, SLOT( channelListCustomPopupMenu( QPoint ) ) );

    mChannelId.clear();
    model = new QStandardItemModel(0, 2, this);
    model->setHeaderData(COLUMN_NAME, Qt::Horizontal, tr("Name"), Qt::DisplayRole);
    model->setHeaderData(COLUMN_POPULARITY, Qt::Horizontal, tr("Popularity"), Qt::DisplayRole);

    treeView->setModel(model);
    treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);

    treeView->setItemDelegate(new ChanGroupDelegate());
    treeView->setRootIsDecorated(true);

    connect(treeView->selectionModel(), SIGNAL(currentRowChanged(QModelIndex,QModelIndex)), this, SLOT(selectChannel(QModelIndex)));

    // hide header and id column
    treeView->setHeaderHidden(true);

    /* Set header resize modes and initial section sizes TreeView*/
    QHeaderView * _header = treeView->header () ;
    _header->setResizeMode ( COLUMN_POPULARITY, QHeaderView::Custom);
    _header->resizeSection ( COLUMN_NAME, 190 );
    
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
    if (!rsChannels->getChannelInfo(mChannelId, ci)) {
        return;
    }

    QMenu contextMnu(this);

    QAction *postchannelAct = new QAction(QIcon(":/images/mail_reply.png"), tr( "Post to Channel" ), &contextMnu);
    connect( postchannelAct , SIGNAL( triggered() ), this, SLOT( createMsg() ) );

    QAction *subscribechannelAct = new QAction(QIcon(":/images/edit_add24.png"), tr( "Subscribe to Channel" ), &contextMnu);
    connect( subscribechannelAct , SIGNAL( triggered() ), this, SLOT( subscribeChannel() ) );

    QAction *unsubscribechannelAct = new QAction(QIcon(":/images/cancel.png"), tr( "Unsubscribe to Channel" ), &contextMnu);
    connect( unsubscribechannelAct , SIGNAL( triggered() ), this, SLOT( unsubscribeChannel() ) );

    QAction *channeldetailsAct = new QAction(QIcon(":/images/info16.png"), tr( "Show Channel Details" ), &contextMnu);
    connect( channeldetailsAct , SIGNAL( triggered() ), this, SLOT( showChannelDetails() ) );

    QAction *restoreKeysAct = new QAction(QIcon(":/images/settings16.png"), tr("Restore Publish Rights for Channel" ), &contextMnu);
    connect( restoreKeysAct , SIGNAL( triggered() ), this, SLOT( restoreChannelKeys() ) );

    QAction *editChannelDetailAct = new QAction(QIcon(":/images/edit_16.png"), tr("Edit Channel Details"), &contextMnu);
    connect( editChannelDetailAct, SIGNAL( triggered() ), this, SLOT( editChannelDetail() ) );

    QAction *shareKeyAct = new QAction(QIcon(":/images/gpgp_key_generate.png"), tr("Share Channel"), &contextMnu);
    connect( shareKeyAct, SIGNAL( triggered() ), this, SLOT( shareKey() ) );

    if ((ci.channelFlags & RS_DISTRIB_PUBLISH) && (ci.channelFlags & RS_DISTRIB_ADMIN)) {
        contextMnu.addAction( postchannelAct );
        contextMnu.addSeparator();
        contextMnu.addAction( editChannelDetailAct);
        contextMnu.addAction( shareKeyAct );
        contextMnu.addAction( channeldetailsAct );
    }
    else if (ci.channelFlags & RS_DISTRIB_PUBLISH) {
        contextMnu.addAction( postchannelAct );
        contextMnu.addSeparator();
        contextMnu.addAction( channeldetailsAct );
        contextMnu.addAction( shareKeyAct );
    } else if (ci.channelFlags & RS_DISTRIB_SUBSCRIBED) {
        contextMnu.addAction( unsubscribechannelAct );
        contextMnu.addSeparator();
        contextMnu.addAction( channeldetailsAct );
    	contextMnu.addAction( restoreKeysAct );
    } else {
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
}

void ChannelFeed::openChat(std::string peerId)
{
}

void ChannelFeed::editChannelDetail(){
    
    EditChanDetails editUi(this, 0, mChannelId);
    editUi.exec();
}

void ChannelFeed::shareKey()
{
    ShareKey shareUi(this, 0, mChannelId);
    shareUi.exec();
}

void ChannelFeed::createMsg()
{
    if (mChannelId.empty()) {
	return;
    }

    CreateChannelMsg *msgDialog = new CreateChannelMsg(mChannelId);
    msgDialog->show();

    /* window will destroy itself! */
}

void ChannelFeed::restoreChannelKeys()
{
    rsChannels->channelRestoreKeys(mChannelId);
}

void ChannelFeed::selectChannel(QModelIndex index)
{
    QStandardItem *itemData = NULL;

    if (index.isValid()) {
        QStandardItem *item = model->itemFromIndex(index);
        if (item && item->parent() != NULL) {
            itemData = item->parent()->child(item->row(), COLUMN_DATA);
        }
    }

    if (itemData) {
        mChannelId = itemData->data(ROLE_ID).toString().toStdString();
    } else {
        mChannelId.clear();
    }

    updateChannelMsgs();
}

void ChannelFeed::updateDisplay()
{
    if (!rsChannels) {
        return;
    }

    std::list<std::string> chanIds;
    std::list<std::string>::iterator it;

    if (rsChannels->channelsChanged(chanIds)) {
        /* update channel list */
        updateChannelList();

        it = std::find(chanIds.begin(), chanIds.end(), mChannelId);
        if (it != chanIds.end()) {
            updateChannelMsgs();
        }
    }
}

void ChannelFeed::updateChannelList()
{
    if (!rsChannels) {
        return;
    }

    std::list<ChannelInfo> channelList;
    std::list<ChannelInfo>::iterator it;

    rsChannels->getChannelList(channelList);

    /* get the ids for our lists */
    std::list<ChannelInfo> adminList;
    std::list<ChannelInfo> subList;
    std::list<ChannelInfo> popList;
    std::list<ChannelInfo> otherList;
    std::multimap<uint32_t, ChannelInfo> popMap;

    for(it = channelList.begin(); it != channelList.end(); it++) {
        /* sort it into Publish (Own), Subscribed, Popular and Other */
        uint32_t flags = it->channelFlags;

        if (flags & RS_DISTRIB_ADMIN) {
            adminList.push_back(*it);
        } else if (flags & RS_DISTRIB_SUBSCRIBED) {
            subList.push_back(*it);
        } else {
            /* rate the others by popularity */
            popMap.insert(std::make_pair(it->pop, *it));
        }
    }

    /* iterate backwards through popMap - take the top 5 or 10% of list */
    uint32_t popCount = 5;
    if (popCount < popMap.size() / 10) {
        popCount = popMap.size() / 10;
    }

    uint32_t i = 0;
    std::multimap<uint32_t, ChannelInfo>::reverse_iterator rit;
    for (rit = popMap.rbegin(); rit != popMap.rend(); rit++) {
        if (i < popCount) {
            popList.push_back(rit->second);
            i++;
        } else {
            otherList.push_back(rit->second);
        }
    }

    /* now we have our lists ---> update entries */

    fillChannelList(OWN, adminList);
    fillChannelList(SUBSCRIBED, subList);
    fillChannelList(POPULAR, popList);
    fillChannelList(OTHER, otherList);

    updateMessageSummaryList("");
}

void ChannelFeed::fillChannelList(int channelItem, std::list<ChannelInfo> &channelInfos)
{
    std::list<ChannelInfo>::iterator iit;

    /* remove rows with groups before adding new ones */
    QStandardItem *groupItem = model->item(channelItem);
    if (groupItem == NULL) {
        return;
    }

    /* iterate all channels */
    for (iit = channelInfos.begin(); iit != channelInfos.end(); iit++) {
#ifdef CHAN_DEBUG
        std::cerr << "ChannelFeed::fillChannelList(): " << channelItem << " - " << iit->channelId << std::endl;
#endif

        ChannelInfo &ci = *iit;
        QString channelId = QString::fromStdString(ci.channelId);

        /* search exisiting channel item */
        int row;
        int rowCount = groupItem->rowCount();
        for (row = 0; row < rowCount; row++) {
            if (groupItem->child(row, COLUMN_DATA)->data(ROLE_ID).toString() == channelId) {
                /* found channel */
                break;
            }
        }

        QStandardItem *chNameItem = new QStandardItem();
        QStandardItem *chPopItem = new QStandardItem();
        if (row < rowCount) {
            chNameItem = groupItem->child(row, COLUMN_NAME);
            chPopItem = groupItem->child(row, COLUMN_POPULARITY);
        } else {
            QList<QStandardItem*> channel;
            chNameItem = new QStandardItem();
            chPopItem = new QStandardItem();

            chNameItem->setSizeHint(QSize(22, 22));

            channel.append(chNameItem);
            channel.append(chPopItem);
            groupItem->appendRow(channel);

            groupItem->child(chNameItem->index().row(), COLUMN_DATA)->setData(channelId, ROLE_ID);
        }

        chNameItem->setText(QString::fromStdWString(ci.channelName));
        groupItem->child(chNameItem->index().row(), COLUMN_DATA)->setData(QString::fromStdWString(ci.channelName), ROLE_CHANNEL_TITLE);
        chNameItem->setToolTip(tr("Popularity: %1\nFetches: %2\nAvailable: %3").arg(QString::number(ci.pop)).arg(9999).arg(9999));

        QPixmap chanImage;
        if (ci.pngImageLen != 0) {
            chanImage.loadFromData(ci.pngChanImage, ci.pngImageLen, "PNG");
        } else {
            chanImage = QPixmap(CHAN_DEFAULT_IMAGE);
        }
        chNameItem->setIcon(QIcon(chanImage));

        /* set Popularity icon */
        int popcount = ci.pop;
        if (popcount == 0) {
            chPopItem->setIcon(QIcon(":/images/hot_0.png"));
        } else if (popcount < 2) {
            chPopItem->setIcon(QIcon(":/images/hot_1.png"));
        } else if (popcount < 4) {
            chPopItem->setIcon(QIcon(":/images/hot_2.png"));
        } else if (popcount < 8) {
            chPopItem->setIcon(QIcon(":/images/hot_3.png"));
        } else if (popcount < 16) {
            chPopItem->setIcon(QIcon(":/images/hot_4.png"));
        } else {
            chPopItem->setIcon(QIcon(":/images/hot_5.png"));
        }
    }

    /* remove all items not in list */
    int row = 0;
    int rowCount = groupItem->rowCount();
    while (row < rowCount) {
        std::string channelId = groupItem->child(row, COLUMN_DATA)->data(ROLE_ID).toString().toStdString();

        for (iit = channelInfos.begin(); iit != channelInfos.end(); iit++) {
            if (iit->channelId == channelId) {
                break;
            }
        }

        if (iit == channelInfos.end()) {
            groupItem->removeRow(row);
            rowCount = groupItem->rowCount();
        } else {
            row++;
        }
    }
}

void ChannelFeed::channelMsgReadSatusChanged(const QString& channelId, const QString& msgId, int status)
{
    updateMessageSummaryList(channelId.toStdString());
}

void ChannelFeed::updateMessageSummaryList(const std::string &channelId)
{
    int channelItems[2] = { OWN, SUBSCRIBED };

    for (int channelItem = 0; channelItem < 2; channelItem++) {
        QStandardItem *groupItem = model->item(channelItem);
        if (groupItem == NULL) {
            continue;
        }

        int row;
        int rowCount = groupItem->rowCount();
        for (row = 0; row < rowCount; row++) {
            std::string rowChannelId = groupItem->child(row, COLUMN_DATA)->data(ROLE_ID).toString().toStdString();
            if (rowChannelId.empty()) {
                continue;
            }

            if (channelId.empty() || rowChannelId == channelId) {
                /* calculate unread messages */
                unsigned int newMessageCount = 0;
                unsigned int unreadMessageCount = 0;
                rsChannels->getMessageCount(rowChannelId, newMessageCount, unreadMessageCount);

                QStandardItem *item = groupItem->child(row, COLUMN_NAME);

                QString title = item->data(ROLE_CHANNEL_TITLE).toString();
                QFont font = item->font();

                if (unreadMessageCount) {
                    title += " (" + QString::number(unreadMessageCount) + ")";
                    font.setBold(true);
                } else {
                    font.setBold(false);
                }

                item->setText(title);
                item->setFont(font);

                if (channelId.empty() == false) {
                    /* calculate only this channel */
                    break;
                }
            }
        }
    }
}

static bool sortChannelMsgSummary(const ChannelMsgSummary &msg1, const ChannelMsgSummary &msg2)
{
    return (msg1.ts < msg2.ts);
}

void ChannelFeed::updateChannelMsgs()
{
    if (!rsChannels) {
        return;
    }

    ChannelInfo ci;
    if (!rsChannels->getChannelInfo(mChannelId, ci)) {
        postButton->setEnabled(false);
        subscribeButton->setEnabled(false);
        unsubscribeButton->setEnabled(false);
        setAllAsReadButton->setEnabled(false);
        nameLabel->setText(tr("No Channel Selected"));
        iconLabel->setPixmap(QPixmap(":/images/channels.png"));
        iconLabel->setEnabled(false);
        return;
    }

    if (ci.pngImageLen != 0) {
        QPixmap chanImage;
        chanImage.loadFromData(ci.pngChanImage, ci.pngImageLen, "PNG");
        iconLabel->setPixmap(chanImage);
        iconLabel->setStyleSheet("QLabel{border: 3px solid white;}");
    } else {
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
    if (ci.channelFlags & RS_DISTRIB_SUBSCRIBED) {
        subscribeButton->setEnabled(false);
        unsubscribeButton->setEnabled(true);
        setAllAsReadButton->setEnabled(true);
    } else {
        subscribeButton->setEnabled(true);
        unsubscribeButton->setEnabled(false);
        setAllAsReadButton->setEnabled(false);
    }

    if (ci.channelFlags & RS_DISTRIB_PUBLISH) {
        postButton->setEnabled(true);
    } else {
        postButton->setEnabled(false);
    }

    /* replace all the messages with new ones */
    std::list<ChanMsgItem *>::iterator mit;
    for (mit = mChanMsgItems.begin(); mit != mChanMsgItems.end(); mit++) {
        delete (*mit);
    }
    mChanMsgItems.clear();

    std::list<ChannelMsgSummary> msgs;
    std::list<ChannelMsgSummary>::iterator it;

    rsChannels->getChannelMsgList(mChannelId, msgs);

    msgs.sort(sortChannelMsgSummary);

    for(it = msgs.begin(); it != msgs.end(); it++) {
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

    if (rsChannels) {
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

    if (rsChannels) {
        rsChannels->channelSubscribe(mChannelId, true);
    }

    updateChannelMsgs();
}

void ChannelFeed::showChannelDetails()
{
    if (mChannelId.empty()) {
	return;
    }

    if (!rsChannels) {
        return;
    }

    ChannelDetails channelui (this);

    channelui.showDetails(mChannelId);
    channelui.exec();
}

void ChannelFeed::setAllAsReadClicked()
{
    if (mChannelId.empty()) {
        return;
    }

    if (!rsChannels) {
        return;
    }

    ChannelInfo ci;
    if (rsChannels->getChannelInfo(mChannelId, ci) == false) {
        return;
    }

    if (ci.channelFlags & RS_DISTRIB_SUBSCRIBED) {
        std::list<ChannelMsgSummary> msgs;
        std::list<ChannelMsgSummary>::iterator it;

        rsChannels->getChannelMsgList(mChannelId, msgs);

        for(it = msgs.begin(); it != msgs.end(); it++) {
            rsChannels->setMessageStatus(mChannelId, it->msgId, CHANNEL_MSG_STATUS_READ, CHANNEL_MSG_STATUS_READ | CHANNEL_MSG_STATUS_UNREAD_BY_USER);
        }
    }
}
