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
#include <QMessageBox>

#include <iostream>
#include <algorithm>
#include <set>
#include <map>

#include "ChannelFeed.h"

#include "feeds/ChanMsgItem.h"
#include "common/PopularityDefs.h"
#include "settings/rsharesettings.h"

#include "channels/CreateChannel.h"
#include "channels/ChannelDetails.h"
#include "channels/CreateChannelMsg.h"
#include "channels/EditChanDetails.h"
#include "channels/ShareKey.h"
#include "notifyqt.h"
#include "RetroShareLink.h"

#define CHAN_DEFAULT_IMAGE ":/images/channels.png"

#define WARNING_LIMIT 3600*24*2

/****
 * #define CHAN_DEBUG
 ***/

/** Constructor */
ChannelFeed::ChannelFeed(QWidget *parent)
: RsAutoUpdatePage(1000,parent)
{
    /* Invoke the Qt Designer generated object setup routine */
    setupUi(this);

    connect(newChannelButton, SIGNAL(clicked()), this, SLOT(createChannel()));
    connect(postButton, SIGNAL(clicked()), this, SLOT(createMsg()));
    connect(subscribeButton, SIGNAL( clicked( void ) ), this, SLOT( subscribeChannel ( void ) ) );
    connect(unsubscribeButton, SIGNAL( clicked( void ) ), this, SLOT( unsubscribeChannel ( void ) ) );
    connect(actionsetAllAsRead, SIGNAL(triggered()), this, SLOT(setAllAsReadClicked()));
    connect(actionEnable_Auto_Download, SIGNAL(triggered()), this, SLOT(toggleAutoDownload()));

    connect(NotifyQt::getInstance(), SIGNAL(channelMsgReadSatusChanged(QString,QString,int)), this, SLOT(channelMsgReadSatusChanged(QString,QString,int)));

    /*************** Setup Left Hand Side (List of Channels) ****************/

    connect(treeWidget, SIGNAL(treeCustomContextMenuRequested(QPoint)), this, SLOT( channelListCustomPopupMenu( QPoint ) ) );
    connect(treeWidget, SIGNAL(treeCurrentItemChanged(QString)), this, SLOT(selectChannel(QString)));

    mChannelId.clear();

    /* Set initial size the splitter */
    QList<int> sizes;
    sizes << 300 << width(); // Qt calculates the right sizes
    splitter->setSizes(sizes);

    /* Initialize group tree */
    treeWidget->initDisplayMenu(displayButton);

    ownChannels = treeWidget->addCategoryItem(tr("Own Channels"), QIcon(), true);
    subcribedChannels = treeWidget->addCategoryItem(tr("Subscribed Channels"), QIcon(), true);
    popularChannels = treeWidget->addCategoryItem(tr("Popular Channels"), QIcon(), false);
    otherChannels = treeWidget->addCategoryItem(tr("Other Channels"), QIcon(), false);

    QMenu * channeloptsmenu = new QMenu();
    channeloptsmenu->addAction(actionsetAllAsRead);
    channeloptsmenu->addAction(actionEnable_Auto_Download);
    channeloptions_Button->setMenu(channeloptsmenu);

    progressLabel->hide();
    progressBar->hide();

    fillThread = NULL;

    //added from ahead
    updateChannelList();
    
    nameLabel->setFont(QFont("MS SANS SERIF", 22));
    nameLabel->setMinimumWidth(20);

    /* load settings */
    processSettings(true);

    updateChannelMsgs();
}

ChannelFeed::~ChannelFeed()
{
    if (fillThread) {
        fillThread->stop();
        delete(fillThread);
        fillThread = NULL;
    }

    // save settings
    processSettings(false);
}

void ChannelFeed::processSettings(bool load)
{
    Settings->beginGroup(QString("ChannelFeed"));

    if (load) {
        // load settings

        // state of splitter
        splitter->restoreState(Settings->value("Splitter").toByteArray());
    } else {
        // save settings

        // state of splitter
        Settings->setValue("Splitter", splitter->saveState());
    }

    treeWidget->processSettings(Settings, load);

    Settings->endGroup();
}

void ChannelFeed::channelListCustomPopupMenu( QPoint /*point*/ )
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
    else if ((ci.channelFlags & RS_DISTRIB_PUBLISH) && (ci.channelFlags & RS_DISTRIB_SUBSCRIBED)) {
        contextMnu.addAction( postchannelAct );
        contextMnu.addSeparator();
        contextMnu.addAction( channeldetailsAct );
        contextMnu.addAction( shareKeyAct );
        contextMnu.addAction( restoreKeysAct );
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

    QAction *action = contextMnu.addAction(QIcon(":/images/copyrslink.png"), tr("Copy RetroShare Link"), this, SLOT(copyChannelLink()));
    action->setEnabled(!mChannelId.empty());

#ifdef CHAN_DEBUG
    contextMnu.addSeparator();
    action = contextMnu.addAction("Generate mass data", this, SLOT(generateMassData()));
    action->setEnabled (!mChannelId.empty() && (ci.channelFlags & RS_DISTRIB_PUBLISH));
#endif

    contextMnu.exec(QCursor::pos());
}

void ChannelFeed::createChannel()
{
	CreateChannel cf (this);
	cf.exec();
}

/*************************************************************************************/
/*************************************************************************************/
/*************************************************************************************/

void ChannelFeed::deleteFeedItem(QWidget */*item*/, uint32_t /*type*/)
{
}

void ChannelFeed::openChat(std::string /*peerId*/)
{
}

void ChannelFeed::editChannelDetail(){
    
    EditChanDetails editUi(this, 0, mChannelId);
    editUi.exec();
}

void ChannelFeed::shareKey()
{
    ShareKey shareUi(this, 0, mChannelId, CHANNEL_KEY_SHARE);
    shareUi.exec();
}

void ChannelFeed::copyChannelLink()
{
    if (mChannelId.empty()) {
        return;
    }

    ChannelInfo ci;
    if (rsChannels->getChannelInfo(mChannelId, ci)) {
        RetroShareLink link;
        if (link.createChannel(ci.channelId, "")) {
            QList<RetroShareLink> urls;
            urls.push_back(link);
            RSLinkClipboard::copyLinks(urls);
        }
    }
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

void ChannelFeed::selectChannel(const QString &id)
{
    mChannelId = id.toStdString();

    bool autoDl = false;
    rsChannels->channelGetAutoDl(mChannelId, autoDl);

    setAutoDownloadButton(autoDl);

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

static void channelInfoToGroupItemInfo(const ChannelInfo &channelInfo, GroupItemInfo &groupItemInfo)
{
	groupItemInfo.id = QString::fromStdString(channelInfo.channelId);
	groupItemInfo.name = QString::fromStdWString(channelInfo.channelName);
	groupItemInfo.description = QString::fromStdWString(channelInfo.channelDesc);
	groupItemInfo.popularity = channelInfo.pop;
	groupItemInfo.lastpost = QDateTime::fromTime_t(channelInfo.lastPost);

	QPixmap chanImage;
	if (channelInfo.pngImageLen) {
		chanImage.loadFromData(channelInfo.pngChanImage, channelInfo.pngImageLen, "PNG");
	} else {
		chanImage = QPixmap(CHAN_DEFAULT_IMAGE);
	}

	groupItemInfo.icon = QIcon(chanImage);
}

void ChannelFeed::updateChannelList()
{
	if (!rsChannels) {
		return;
	}

	std::list<ChannelInfo> channelList;
	std::list<ChannelInfo>::iterator it;
	rsChannels->getChannelList(channelList);

	std::list<std::string> keysAvailable;
	std::list<std::string>::iterator keyIt;
	rsChannels->getPubKeysAvailableGrpIds(keysAvailable);

	/* get the ids for our lists */
	QList<GroupItemInfo> adminList;
	QList<GroupItemInfo> subList;
	QList<GroupItemInfo> popList;
	QList<GroupItemInfo> otherList;
	std::multimap<uint32_t, GroupItemInfo> popMap;

	for(it = channelList.begin(); it != channelList.end(); it++) {
		/* sort it into Publish (Own), Subscribed, Popular and Other */
		uint32_t flags = it->channelFlags;

		GroupItemInfo groupItemInfo;
		channelInfoToGroupItemInfo(*it, groupItemInfo);

		if ((flags & RS_DISTRIB_ADMIN) && (flags & RS_DISTRIB_PUBLISH) && (flags & RS_DISTRIB_SUBSCRIBED)) {
			adminList.push_back(groupItemInfo);
		} else {
			for (keyIt = keysAvailable.begin(); keyIt != keysAvailable.end(); keyIt++) {
				if (it->channelId == *keyIt) {
					/* Found Key, set title text to bold and colored blue */
					groupItemInfo.privatekey = true;
					break;
				}
			}

			if ((flags & RS_DISTRIB_SUBSCRIBED) || ((flags & RS_DISTRIB_SUBSCRIBED) && (flags & RS_DISTRIB_PUBLISH)) ) {
				subList.push_back(groupItemInfo);
			} else {
				/* rate the others by popularity */
				popMap.insert(std::make_pair(it->pop, groupItemInfo));
			}
		}
	}

	/* iterate backwards through popMap - take the top 5 or 10% of list */
	uint32_t popCount = 5;
	if (popCount < popMap.size() / 10) {
		popCount = popMap.size() / 10;
	}

	uint32_t i = 0;
	std::multimap<uint32_t, GroupItemInfo>::reverse_iterator rit;
	for (rit = popMap.rbegin(); rit != popMap.rend(); rit++) {
		if (i < popCount) {
			popList.push_back(rit->second);
			i++;
		} else {
			otherList.push_back(rit->second);
		}
	}

    /* now we have our lists ---> update entries */

	treeWidget->fillGroupItems(ownChannels, adminList);
	treeWidget->fillGroupItems(subcribedChannels, subList);
	treeWidget->fillGroupItems(popularChannels, popList);
	treeWidget->fillGroupItems(otherChannels, otherList);

	updateMessageSummaryList("");
}


void ChannelFeed::channelMsgReadSatusChanged(const QString& channelId, const QString& /*msgId*/, int /*status*/)
{
    updateMessageSummaryList(channelId.toStdString());
}

void ChannelFeed::updateMessageSummaryList(const std::string &channelId)
{
	QTreeWidgetItem *items[2] = { ownChannels, subcribedChannels };

	for (int item = 0; item < 2; item++) {
		int child;
		int childCount = items[item]->childCount();
		for (child = 0; child < childCount; child++) {
			QTreeWidgetItem *childItem = items[item]->child(child);
			std::string childId = treeWidget->itemId(childItem).toStdString();
			if (childId.empty()) {
				continue;
			}

			if (channelId.empty() || childId == channelId) {
				/* Calculate unread messages */
				unsigned int newMessageCount = 0;
				unsigned int unreadMessageCount = 0;
				rsChannels->getMessageCount(childId, newMessageCount, unreadMessageCount);

				treeWidget->setUnreadCount(childItem, unreadMessageCount);

				if (channelId.empty() == false) {
					/* Calculate only this channel */
					break;
				}
			}
		}
	}
}

static bool sortChannelMsgSummary(const ChannelMsgSummary &msg1, const ChannelMsgSummary &msg2)
{
    return (msg1.ts > msg2.ts);
}

void ChannelFeed::updateChannelMsgs()
{
    if (fillThread) {
#ifdef CHAN_DEBUG
        std::cerr << "ChannelFeed::updateChannelMsgs() stop current fill thread" << std::endl;
#endif
        // stop current fill thread
        fillThread->stop();
        delete(fillThread);
        fillThread = NULL;

        progressLabel->hide();
        progressBar->hide();
    }

    if (!rsChannels) {
        return;
    }

    /* replace all the messages with new ones */
    QList<ChanMsgItem *>::iterator mit;
    for (mit = mChanMsgItems.begin(); mit != mChanMsgItems.end(); mit++) {
        delete (*mit);
    }
    mChanMsgItems.clear();

    ChannelInfo ci;
    if (!rsChannels->getChannelInfo(mChannelId, ci)) {
        postButton->setEnabled(false);
        subscribeButton->setEnabled(false);
        unsubscribeButton->setEnabled(false);
        subscribeButton->hide();
        unsubscribeButton->hide();
        actionsetAllAsRead->setEnabled(false);
        actionEnable_Auto_Download->setEnabled(false);
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
    QString channelStr("<span style=\"font-size:22pt; font-weight:500;color:#4F4F4F;\">%1</span>");

    /* set Channel name */
    QString cname = QString::fromStdWString(ci.channelName);
    nameLabel->setText(channelStr.arg(cname));

    /* do buttons */
    if (ci.channelFlags & RS_DISTRIB_SUBSCRIBED) {
        subscribeButton->setEnabled(false);
        subscribeButton->hide();
        unsubscribeButton->show();
        unsubscribeButton->setEnabled(true);
        actionsetAllAsRead->setEnabled(true);
    } else {
        subscribeButton->setEnabled(true);
        subscribeButton->show();
        unsubscribeButton->setEnabled(false);
        unsubscribeButton->hide();
        actionsetAllAsRead->setEnabled(false);
    }

    if (ci.channelFlags & RS_DISTRIB_PUBLISH) {
        postButton->setEnabled(true);
    } else {
        postButton->setEnabled(false);
    }

    if (!(ci.channelFlags & RS_DISTRIB_ADMIN) &&
         (ci.channelFlags & RS_DISTRIB_SUBSCRIBED)) {
        actionEnable_Auto_Download->setEnabled(true);
    } else {
        actionEnable_Auto_Download->setEnabled(false);
    }

    progressLabel->show();
    progressBar->reset();
    progressBar->show();

    // create fill thread
    fillThread = new ChannelFillThread(this, mChannelId);

    // connect thread
    connect(fillThread, SIGNAL(finished()), this, SLOT(fillThreadFinished()), Qt::BlockingQueuedConnection);
    connect(fillThread, SIGNAL(progress(int,int)), this, SLOT(fillThreadProgress(int,int)));
    connect(fillThread, SIGNAL(addMsg(QString,QString)), this, SLOT(fillThreadAddMsg(QString,QString)), Qt::BlockingQueuedConnection);

#ifdef DEBUG_FORUMS
    std::cerr << "ChannelFeed::updateChannelMsgs() Start fill thread" << std::endl;
#endif

    // start thread
    fillThread->start();
}

void ChannelFeed::fillThreadFinished()
{
#ifdef DEBUG_FORUMS
    std::cerr << "ChannelFeed::fillThreadFinished" << std::endl;
#endif

    // thread has finished
    ChannelFillThread *thread = dynamic_cast<ChannelFillThread*>(sender());
    if (thread) {
        if (thread == fillThread) {
            // current thread has finished, hide progressbar and release thread
            progressBar->hide();
            progressLabel->hide();
            fillThread = NULL;
        }

        if (thread->wasStopped()) {
            // thread was stopped
#ifdef DEBUG_FORUMS
            std::cerr << "ChannelFeed::fillThreadFinished Thread was stopped" << std::endl;
#endif
        }

#ifdef DEBUG_FORUMS
        std::cerr << "ChannelFeed::fillThreadFinished Delete thread" << std::endl;
#endif

        thread->deleteLater();
        thread = NULL;
    }

#ifdef DEBUG_FORUMS
    std::cerr << "ChannelFeed::fillThreadFinished done" << std::endl;
#endif
}

void ChannelFeed::fillThreadProgress(int current, int count)
{
    // show fill progress
    if (count) {
        progressBar->setValue(current * progressBar->maximum() / count);
    }
}

void ChannelFeed::fillThreadAddMsg(const QString &channelId, const QString &channelMsgId)
{
    if (sender() == fillThread) {
        ChanMsgItem *cmi = new ChanMsgItem(this, 0, channelId.toStdString(), channelMsgId.toStdString(), true);
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
        rsChannels->channelSubscribe(mChannelId, false, false);
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
        rsChannels->channelSubscribe(mChannelId, true, false);
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

void ChannelFeed::toggleAutoDownload(){

	if(mChannelId.empty())
		return;

	bool autoDl = true;

	if(rsChannels->channelGetAutoDl(mChannelId, autoDl)){

		// if auto dl is set true, then set false
		if(autoDl){
			rsChannels->channelSetAutoDl(mChannelId, false);
		}else{
			rsChannels->channelSetAutoDl(mChannelId, true);
		}
		setAutoDownloadButton(!autoDl);
	}
	else{
		std::cerr << "Auto Download failed to set"
				  << std::endl;
	}
}

bool ChannelFeed::navigate(const std::string& channelId, const std::string& msgId)
{
	if (channelId.empty()) {
		return false;
	}

	if (treeWidget->activateId(QString::fromStdString(channelId), msgId.empty()) == NULL) {
		return false;
	}

	/* Messages are filled in selectChannel */
	if (mChannelId != channelId) {
		return false;
	}

	if (msgId.empty()) {
		return true;
	}

	/* Search exisiting item */
	QList<ChanMsgItem*>::iterator mit;
	for (mit = mChanMsgItems.begin(); mit != mChanMsgItems.end(); mit++) {
		ChanMsgItem *item = *mit;
		if (item->msgId() == msgId) {
			// the next two lines are necessary to calculate the layout of the widgets in the scroll area (maybe there is a better solution)
			item->show();
			QCoreApplication::processEvents();

			scrollArea->ensureWidgetVisible(item, 0, 0);
			return true;
		}
	}

	return false;
}

void ChannelFeed::setAutoDownloadButton(bool autoDl)
{
	if (autoDl) {
		actionEnable_Auto_Download->setText(tr("Disable Auto-Download"));
	}else{
		actionEnable_Auto_Download->setText(tr("Enable Auto-Download"));
	}
}

void ChannelFeed::generateMassData()
{
#ifdef CHAN_DEBUG
    if (mChannelId.empty ()) {
        return;
    }

    if (QMessageBox::question(this, "Generate mass data", "Do you really want to generate mass data ?", QMessageBox::Yes|QMessageBox::No, QMessageBox::No) == QMessageBox::No) {
        return;
    }

    for (int thread = 1; thread < 1000; thread++) {
        ChannelMsgInfo msgInfo;
        msgInfo.channelId = mChannelId;
        msgInfo.subject = QString("Test %1").arg(thread, 3, 10, QChar('0')).toStdWString();
        msgInfo.msg = QString("That is only a test").toStdWString();

        if (rsChannels->ChannelMessageSend(msgInfo) == false) {
            return;
        }
    }
#endif
}

// ForumsFillThread
ChannelFillThread::ChannelFillThread(ChannelFeed *parent, const std::string &channelId)
    : QThread(parent)
{
    stopped = false;
    this->channelId = channelId;
}

ChannelFillThread::~ChannelFillThread()
{
#ifdef CHAN_DEBUG
    std::cerr << "ChannelFillThread::~ChannelFillThread" << std::endl;
#endif
}

void ChannelFillThread::stop()
{
    disconnect();
    stopped = true;
    QApplication::processEvents();
    wait();
}

void ChannelFillThread::run()
{
#ifdef CHAN_DEBUG
    std::cerr << "ChannelFillThread::run()" << std::endl;
#endif

    std::list<ChannelMsgSummary> msgs;
    std::list<ChannelMsgSummary>::iterator it;
    rsChannels->getChannelMsgList(channelId, msgs);

    msgs.sort(sortChannelMsgSummary);

    int count = msgs.size();
    int pos = 0;

    for (it = msgs.begin(); it != msgs.end(); it++) {
        if (stopped) {
            break;
        }

        emit addMsg(QString::fromStdString(channelId), QString::fromStdString(it->msgId));
        emit progress(++pos, count);
    }

#ifdef CHAN_DEBUG
    std::cerr << "ChannelFillThread::run() stopped: " << (wasStopped() ? "yes" : "no") << std::endl;
#endif
}
