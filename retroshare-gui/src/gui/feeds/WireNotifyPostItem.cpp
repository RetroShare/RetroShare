/*******************************************************************************
 * gui/feeds/WireNotifyPostItem.cpp                                            *
 *                                                                             *
 * Copyright (c) 2012, Robert Fernie   <retroshare.project@gmail.com>          *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Affero General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/

#include "WireNotifyPostItem.h"
#include "ui_WireNotifyPostItem.h"

#include "FeedHolder.h"
#include "util/qtthreadsutils.h"
#include "gui/RetroShareLink.h"
#include "gui/common/FilesDefs.h"

#include <iostream>
#include <cmath>

WireNotifyPostItem::WireNotifyPostItem(FeedHolder *feedHolder, uint32_t feedId, const RsGxsGroupId &groupId, const RsGxsMessageId &messageId, bool isHome, bool autoUpdate, const std::set<RsGxsMessageId> &older_versions) :
    GxsFeedItem(feedHolder, feedId, groupId, messageId, isHome, rsWire, autoUpdate) // this one should be in GxsFeedItem
{
    mPulse.mMeta.mMsgId = messageId; // useful for uniqueIdentifer() before the post is loaded

    QVector<RsGxsMessageId> v;
    //bool self = false;

    for(std::set<RsGxsMessageId>::const_iterator it(older_versions.begin());it!=older_versions.end();++it)
        v.push_back(*it) ;

    if(older_versions.find(messageId) == older_versions.end())
        v.push_back(messageId);

    setMessageVersions(v) ;
    setup();

    loadGroup();
}

WireNotifyPostItem::WireNotifyPostItem(FeedHolder *feedHolder, uint32_t feedId, const RsGroupMetaData& group_meta, const RsGxsMessageId &messageId, bool isHome, bool autoUpdate,const std::set<RsGxsMessageId>& older_versions) :
    GxsFeedItem(feedHolder, feedId, group_meta.mGroupId, messageId, isHome, rsWire, autoUpdate),
    mGroupMeta(group_meta)
{
//    mLoadingGroup = false;
//    mLoadingMessage = false;
//    mLoadingComment = false;

//    mPost.mMeta.mMsgId = messageId; // useful for uniqueIdentifer() before the post is loaded
//    mPost.mMeta.mGroupId = mGroupMeta.mGroupId;

//	QVector<RsGxsMessageId> v;
//	//bool self = false;

//	for(std::set<RsGxsMessageId>::const_iterator it(older_versions.begin());it!=older_versions.end();++it)
//		v.push_back(*it) ;

//	if(older_versions.find(messageId) == older_versions.end())
//		v.push_back(messageId);

//	setMessageVersions(v) ;
    setup();

    // no call to loadGroup() here because we have it already.
}

void WireNotifyPostItem::paintEvent(QPaintEvent *e)
{
    /* This method employs a trick to trigger a deferred loading. The post and group is requested only
     * when actually displayed on the screen. */

//	if(!mLoaded)
//	{
//		mLoaded = true ;

//        std::set<RsGxsMessageId> older_versions;	// not so nice. We need to use std::set everywhere
//        for(auto& m:messageVersions())
//            older_versions.insert(m);

//		fill();
//		requestMessage();
//		requestComment();
//	}

//	GxsFeedItem::paintEvent(e) ;
}

WireNotifyPostItem::~WireNotifyPostItem()
{
    delete ui;
}

void WireNotifyPostItem::setup()
{
    /* Invoke the Qt Designer generated object setup routine */

    ui = new Ui::WireNotifyPostItem;
    ui->setupUi(this);

    // Manually set icons to allow to use clever resource sharing that is missing in Qt for Icons loaded from Qt resource file.
    // This is particularly important here because a wire may contain many posts, so duplicating the QImages here is deadly for the
    // memory.

    ui->logoLabel->setPixmap(FilesDefs::getPixmapFromQtResourcePath(":/images/thumb-default-video.png"));
    //ui->warn_image_label->setPixmap(FilesDefs::getPixmapFromQtResourcePath(":/images/status_unknown.png"));
    ui->readButton->setIcon(FilesDefs::getIconFromQtResourcePath(":/images/message-state-unread.png"));
    ui->voteUpButton->setIcon(FilesDefs::getIconFromQtResourcePath(":/images/vote_up.png"));
    ui->voteDownButton->setIcon(FilesDefs::getIconFromQtResourcePath(":/images/vote_down.png"));
    ui->downloadButton->setIcon(FilesDefs::getIconFromQtResourcePath(":/icons/png/download.png"));
    ui->playButton->setIcon(FilesDefs::getIconFromQtResourcePath(":/icons/png/play.png"));
    ui->commentButton->setIcon(FilesDefs::getIconFromQtResourcePath(":/icons/png/comment.png"));
    //ui->editButton->setIcon(FilesDefs::getIconFromQtResourcePath(":/icons/png/pencil-edit-button.png"));
    ui->copyLinkButton->setIcon(FilesDefs::getIconFromQtResourcePath(":/icons/png/copy.png"));
    ui->expandButton->setIcon(FilesDefs::getIconFromQtResourcePath(":/icons/png/down-arrow.png"));
    ui->readAndClearButton->setIcon(FilesDefs::getIconFromQtResourcePath(":/icons/png/correct.png"));
    ui->clearButton->setIcon(FilesDefs::getIconFromQtResourcePath(":/icons/png/exit2.png"));

    setAttribute(Qt::WA_DeleteOnClose, true);

    mInFill = false;
    mCloseOnRead = false;
    mLoaded = false;

    /* clear ui */
    ui->titleLabel->setText(tr("Loading..."));
    ui->datetimelabel->clear();
    ui->filelabel->clear();
    ui->newCommentLabel->hide();
    ui->commLabel->hide();

    /* general ones */
    connect(ui->expandButton, SIGNAL(clicked()), this, SLOT(toggle()));
    connect(ui->clearButton, SIGNAL(clicked()), this, SLOT(removeItem()));

    /* specific */
    connect(ui->readAndClearButton, SIGNAL(clicked()), this, SLOT(readAndClearItem()));
    connect(ui->unsubscribeButton, SIGNAL(clicked()), this, SLOT(unsubscribeChannel()));

    connect(ui->downloadButton, SIGNAL(clicked()), this, SLOT(download()));
    // HACK FOR NOW.
    ui->commentButton->hide();// hidden until properly enabled.
    connect(ui->commentButton, SIGNAL(clicked()), this, SLOT(loadComments()));

    connect(ui->playButton, SIGNAL(clicked()), this, SLOT(play(void)));
    //connect(ui->editButton, SIGNAL(clicked()), this, SLOT(edit(void)));
    connect(ui->copyLinkButton, SIGNAL(clicked()), this, SLOT(copyMessageLink()));

    connect(ui->readButton, SIGNAL(toggled(bool)), this, SLOT(readToggled(bool)));

    // hide voting buttons, backend is not implemented yet
    ui->voteUpButton->hide();
    ui->voteDownButton->hide();
    //connect(ui-> voteUpButton, SIGNAL(clicked()), this, SLOT(makeUpVote()));
    //connect(ui->voteDownButton, SIGNAL(clicked()), this, SLOT(makeDownVote()));

    ui->scoreLabel->hide();

    // hide unsubscribe button not necessary
    ui->unsubscribeButton->hide();

    ui->downloadButton->hide();
    ui->playButton->hide();
    //ui->warn_image_label->hide();
    //ui->warning_label->hide();

    ui->titleLabel->setMinimumWidth(100);
    //ui->subjectLabel->setMinimumWidth(100);
    //ui->warning_label->setMinimumWidth(100);

    ui->feedFrame->setProperty("new", false);
    ui->feedFrame->style()->unpolish(ui->feedFrame);
    ui->feedFrame->style()->polish(  ui->feedFrame);

    ui->expandFrame->hide();
}

void WireNotifyPostItem::expandFill(bool first)
{
    GxsFeedItem::expandFill(first);

    if (first) {
//		fillExpandFrame();
    }
}

QString WireNotifyPostItem::messageName()
{
    return QString::fromUtf8(mPulse.mMeta.mMsgName.c_str());
}

void WireNotifyPostItem::loadMessage()
{
#ifdef DEBUG_ITEM
    std::cerr << "WireNotifyPostItem::loadMessage()";
    std::cerr << std::endl;
#endif
    mLoadingMessage = true;

    RsThread::async([this]()
    {
        // 1 - get group data

        std::list<RsGxsGroupId> groupIds;
        std::list<RsWirePulseSPtr> pulses;
//		std::vector<RsGxsComment> comments;
//		std::vector<RsGxsVote> votes;
        groupIds.push_back(groupId());
        if(! rsWire->getPulsesForGroups(groupIds, pulses))
        {
            RsErr() << "WireNotifyPostItem::loadGroup() ERROR getting data" << std::endl;
            return;
        }

        if (pulses.size() == 1)
        {
#ifdef DEBUG_ITEM
            std::cerr << (void*)this << ": Obtained post, with msgId = " << posts[0].mMeta.mMsgId << std::endl;
#endif
            const RsWirePulse& pulse(*pulses.front());

            RsQThreadUtils::postToObject( [pulse,this]()
            {
                setPost(pulse);
                mLoadingMessage = false;
            }, this );
        }
//        else if(comments.size() == 1)
//        {
//            const RsGxsComment& cmt = comments[0];
//#ifdef DEBUG_ITEM
//            std::cerr << (void*)this << ": Obtained comment, setting messageId to threadID = " << cmt.mMeta.mThreadId << std::endl;
//#endif

//            RsQThreadUtils::postToObject( [cmt,this]()
//            {
//                ui->newCommentLabel->show();
//                ui->commLabel->show();
//                ui->commLabel->setText(QString::fromUtf8(cmt.mComment.c_str()));

//                //Change this item to be uploaded with thread element.
//                setMessageId(cmt.mMeta.mThreadId);
//                requestMessage();

//                mLoadingMessage = false;
//            }, this );

//        }
//        else
//        {
//#ifdef DEBUG_ITEM
//            std::cerr << "WireNotifyPostItem::loadMessage() Wrong number of Items. Remove It.";
//            std::cerr << std::endl;
//#endif

//            RsQThreadUtils::postToObject( [this]()
//            {
//                removeItem();
//                mLoadingMessage = false;
//            }, this );
//        }
    });
}

void WireNotifyPostItem::loadComment()
{
#ifdef DEBUG_ITEM
    std::cerr << "WireNotifyPostItem::loadComment()";
    std::cerr << std::endl;
#endif
//    mLoadingComment = true;

//    RsThread::async([this]()
//    {
//        // 1 - get group data

//        std::set<RsGxsMessageId> msgIds;

//        for(auto MsgId: messageVersions())
//            msgIds.insert(MsgId);

//        std::vector<RsGxsChannelPost> posts;
//        std::vector<RsGxsComment> comments;

//        if(! rsGxsChannels->getChannelComments( groupId(),msgIds,comments))
//        {
//            RsErr() << "GxsGxsChannelGroupItem::loadGroup() ERROR getting data" << std::endl;
//            return;
//        }

//        int comNb = comments.size();

//        RsQThreadUtils::postToObject( [comNb,this]()
//        {
//            QString sComButText = tr("Comment");
//            if (comNb == 1)
//                sComButText = sComButText.append("(1)");
//            else if(comNb > 1)
//                sComButText = tr("Comments ").append("(%1)").arg(comNb);

//            ui->commentButton->setText(sComButText);
//            mLoadingComment = false;

//        }, this );
//    });
}

void WireNotifyPostItem::loadGroup()
{
#ifdef DEBUG_ITEM
    std::cerr << "WireNotifyGroupItem::loadGroup()";
    std::cerr << std::endl;
#endif
    mLoadingGroup = true;

    RsThread::async([this]()
    {
        // 1 - get group data

//        std::vector<RsWireGroup> groups;
//        std::list<std::shared_ptr<RsWireGroup>> groups;
        const std::list<RsGxsGroupId> groupIds = { groupId() };
        std::vector<RsWireGroup> groups;
        if(!rsWire->getGroups(groupIds,groups))	// would be better to call channel Summaries for a single group
        {
            RsErr() << "WireNotifyPostItem::loadGroup() ERROR getting data" << std::endl;
            return;
        }

        if (groups.size() != 1)
        {
            std::cerr << "WireNotifyPostItem::loadGroup() Wrong number of Items";
            std::cerr << std::endl;
            return;
        }
        RsWireGroup group = groups[0];

        RsQThreadUtils::postToObject( [group,this]()
        {
            /* Here it goes any code you want to be executed on the Qt Gui
             * thread, for example to update the data model with new information
             * after a blocking call to RetroShare API complete */

            mGroupMeta = group.mMeta;
            mLoadingGroup = false;

        }, this );
    });
}

bool WireNotifyPostItem::setPost(const RsWirePulse &pulse, bool doFill)
{
    if (groupId() != pulse.mMeta.mGroupId || messageId() != pulse.mMeta.mMsgId) {
        std::cerr << "WireNotifyPostItem::setPost() - Wrong id, cannot set post";
        std::cerr << std::endl;
        return false;
    }

    mPulse = pulse;

    if (doFill) {
//        fill();
        std::cout<<"filling needs to be implemented"<<std::endl;
    }

//    updateItem();

    return true;
}



//void WireNotifyPostItem::updateItem()
//{
//    /* fill in */

//#ifdef DEBUG_ITEM
//    std::cerr << "WireNotifyPostItem::updateItem()";
//    std::cerr << std::endl;
//#endif

//    int msec_rate = 10000;

//    int downloadCount = 0;
//    int downloadStartable = 0;
//    int playCount = 0;
//    int playStartable = 0;
//    bool startable;
//    bool loopAgain = false;

//    /* Very slow Tick to check when all files are downloaded */
//    std::list<SubFileItem *>::iterator it;
//    for(it = mFileItems.begin(); it != mFileItems.end(); ++it)
//    {
//        SubFileItem *item = *it;

//        if (item->isDownloadable(startable)) {
//            ++downloadCount;
//            if (startable) {
//                ++downloadStartable;
//            }
//        }
//        if (item->isPlayable(startable)) {
//            ++playCount;
//            if (startable) {
//                ++playStartable;
//            }
//        }

//        if (!item->done())
//        {
//            /* loop again */
//            loopAgain = true;
//        }
//    }

//    if (downloadCount) {
//        ui->downloadButton->show();

//        if (downloadStartable) {
//            ui->downloadButton->setEnabled(true);
//        } else {
//            ui->downloadButton->setEnabled(false);
//        }
//    } else {
//        ui->downloadButton->hide();
//    }
//    if (playCount) {
//        /* one file is playable */
//        ui->playButton->show();

//        if (playStartable == 1) {
//            ui->playButton->setEnabled(true);
//        } else {
//            ui->playButton->setEnabled(false);
//        }
//    } else {
//        ui->playButton->hide();
//    }

//    if (loopAgain) {
//        QTimer::singleShot( msec_rate, this, SLOT(updateItem(void)));
//    }

//    // HACK TO DISPLAY COMMENT BUTTON FOR NOW.
//    //downloadButton->show();
//    //downloadButton->setEnabled(true);
//}

QString WireNotifyPostItem::groupName()
{
    return QString::fromUtf8(mGroupMeta.mGroupName.c_str());
}

void WireNotifyPostItem::doExpand(bool open)
{
    if (mFeedHolder)
    {
        mFeedHolder->lockLayout(this, true);
    }

    if (open)
    {
        ui->expandFrame->show();
        ui->expandButton->setIcon(FilesDefs::getIconFromQtResourcePath(QString(":/icons/png/up-arrow.png")));
        ui->expandButton->setToolTip(tr("Hide"));

//        readToggled(false);
    }
    else
    {
        ui->expandFrame->hide();
        ui->expandButton->setIcon(FilesDefs::getIconFromQtResourcePath(QString(":/icons/png/down-arrow.png")));
        ui->expandButton->setToolTip(tr("Expand"));
    }

    emit sizeChanged(this);

    if (mFeedHolder)
    {
        mFeedHolder->lockLayout(this, false);
    }
}
