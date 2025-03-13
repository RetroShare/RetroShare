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

#include <QStyle>

#include "WireNotifyPostItem.h"
#include "ui_WireNotifyPostItem.h"

#include "FeedHolder.h"
#include "util/qtthreadsutils.h"
#include "gui/RetroShareLink.h"
#include "gui/common/FilesDefs.h"
#include "gui/gxs/GxsIdDetails.h"
#include "util/stringutil.h"
#include "util/DateTime.h"
#include "util/misc.h"

#include <iostream>
#include <cmath>

WireNotifyPostItem::WireNotifyPostItem(FeedHolder *feedHolder, uint32_t feedId, const RsGroupMetaData& group_meta, const RsGxsMessageId &messageId, bool isHome, bool autoUpdate,const std::set<RsGxsMessageId>& older_versions) :
    GxsFeedItem(feedHolder, feedId, group_meta.mGroupId, messageId, isHome, rsWire, autoUpdate),
    mGroupMeta(group_meta)
{
    mLoadingGroup = false;
    mLoadingMessage = false;
    mLoadingComment = false;

    mPulse.mMeta.mMsgId = messageId; // useful for uniqueIdentifer() before the post is loaded
    mPulse.mMeta.mGroupId = mGroupMeta.mGroupId;

	QVector<RsGxsMessageId> v;
	//bool self = false;

	for(std::set<RsGxsMessageId>::const_iterator it(older_versions.begin());it!=older_versions.end();++it)
		v.push_back(*it) ;

	if(older_versions.find(messageId) == older_versions.end())
		v.push_back(messageId);

	setMessageVersions(v) ;
	setup();

    // no call to loadGroup() here because we have it already.
}

WireNotifyPostItem::WireNotifyPostItem(FeedHolder *feedHolder, uint32_t feedId, const RsGxsGroupId& groupId, const RsGxsMessageId &messageId, bool isHome, bool autoUpdate, const std::set<RsGxsMessageId> &older_versions) :
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

void WireNotifyPostItem::paintEvent(QPaintEvent *e)
{
    /* This method employs a trick to trigger a deferred loading. The post and group is requested only
     * when actually displayed on the screen. */

    if(!mLoaded)
    {
        mLoaded = true ;

        std::set<RsGxsMessageId> older_versions;	// not so nice. We need to use std::set everywhere
        for(auto& m:messageVersions())
            older_versions.insert(m);

        fill();
        requestMessage();
        requestComment();
    }

    GxsFeedItem::paintEvent(e) ;
}

WireNotifyPostItem::~WireNotifyPostItem()
{
    auto timeout = std::chrono::steady_clock::now() + std::chrono::milliseconds(300);

    while( (mLoadingGroup || mLoadingMessage || mLoadingComment)
           && std::chrono::steady_clock::now() < timeout)
    {
        RsDbg() << __PRETTY_FUNCTION__ << " is Waiting for "
                << (mLoadingGroup ? "Group " : "")
                << (mLoadingMessage ? "Message " : "")
                << (mLoadingComment ? "Comment " : "")
                << "loading." << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    delete ui;
}

bool WireNotifyPostItem::isUnread() const
{
	return IS_MSG_UNREAD(mPulse.mMeta.mMsgStatus) ;
}

void WireNotifyPostItem::setup()
{
    /* Invoke the Qt Designer generated object setup routine */

    ui = new Ui::WireNotifyPostItem;
    ui->setupUi(this);

    // Manually set icons to allow to use clever resource sharing that is missing in Qt for Icons loaded from Qt resource file.
    // This is particularly important here because a wire may contain many posts, so duplicating the QImages here is deadly for the
    // memory.

//    ui->logoLabel->setPixmap(FilesDefs::getPixmapFromQtResourcePath(":/images/thumb-default-video.png"));
    //ui->warn_image_label->setPixmap(FilesDefs::getPixmapFromQtResourcePath(":/images/status_unknown.png"));
    ui->readButton->setIcon(FilesDefs::getIconFromQtResourcePath(":/images/message-state-unread.png"));
//    ui->voteUpButton->setIcon(FilesDefs::getIconFromQtResourcePath(":/images/vote_up.png"));
//    ui->voteDownButton->setIcon(FilesDefs::getIconFromQtResourcePath(":/images/vote_down.png"));
//    ui->downloadButton->setIcon(FilesDefs::getIconFromQtResourcePath(":/icons/png/download.png"));
//    ui->playButton->setIcon(FilesDefs::getIconFromQtResourcePath(":/icons/png/play.png"));
    ui->commentButton->setIcon(FilesDefs::getIconFromQtResourcePath(":/icons/png/comment.png"));
    //ui->editButton->setIcon(FilesDefs::getIconFromQtResourcePath(":/icons/png/pencil-edit-button.png"));
//    ui->copyLinkButton->setIcon(FilesDefs::getIconFromQtResourcePath(":/icons/png/copy.png"));
//    ui->expandButton->setIcon(FilesDefs::getIconFromQtResourcePath(":/icons/png/down-arrow.png"));
//    ui->readAndClearButton->setIcon(FilesDefs::getIconFromQtResourcePath(":/icons/png/correct.png"));
//    ui->clearButton->setIcon(FilesDefs::getIconFromQtResourcePath(":/icons/png/exit2.png"));

    setAttribute(Qt::WA_DeleteOnClose, true);

    mInFill = false;
    mCloseOnRead = false;
    mLoaded = false;

    /* clear ui */
    ui->titleLabel->setText(tr("Loading..."));
	ui->titleLabel->setOpenExternalLinks(false); //To get linkActivated working
	connect(ui->titleLabel, SIGNAL(linkActivated(QString)), this, SLOT(on_linkActivated(QString)));
    ui->datetimelabel->clear();
    ui->filelabel->clear();
//    ui->newCommentLabel->hide();
//    ui->commLabel->hide();

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
    ui->subjectLabel->setMinimumWidth(100);
    //ui->warning_label->setMinimumWidth(100);

    ui->feedFrame->setProperty("new", false);
    ui->feedFrame->style()->unpolish(ui->feedFrame);
    ui->feedFrame->style()->polish(  ui->feedFrame);

    ui->expandFrame->hide();
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
        fill();
        std::cout<<"filling needs to be implemented"<<std::endl;
    }

//    updateItem();

    return true;
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
            std::cerr << (void*)this << ": Obtained post, with msgId = " << pulses[0].mMeta.mMsgId << std::endl;
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
			setGroup(group);

        }, this );
    });
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

void WireNotifyPostItem::fill()
{
    /* fill in */

//	if (isLoading()) {
    //	/* Wait for all requests */
        //return;
//	}

#ifdef DEBUG_ITEM
    std::cerr << "WireNotifyPostItem::fill()";
    std::cerr << std::endl;
#endif

    std::cout<<"*************************************************filling************************************"<<std::endl;
    mInFill = true;

    QString title;
    QString msgText;

    if (!mIsHome)
    {
        if (mCloseOnRead && !IS_MSG_NEW(mPulse.mMeta.mMsgStatus)) {
            removeItem();
        }

//        title = tr("Wire Feed") + ": ";
//        RetroShareLink link = RetroShareLink::createGxsGroupLink(RetroShareLink::TYPE_WIRE, mPulse.mMeta.mGroupId, groupName());
//        title += link.toHtml();
//        ui->titleLabel->setText(title);

        msgText = tr("Pulse") + ": ";
        RetroShareLink msgLink = RetroShareLink::createGxsMessageLink(RetroShareLink::TYPE_WIRE, mPulse.mMeta.mGroupId, mPulse.mMeta.mMsgId, messageName());
        msgText += msgLink.toHtml();
        ui->subjectLabel->setText(msgText);

        if (IS_GROUP_SUBSCRIBED(mGroupMeta.mSubscribeFlags) || IS_GROUP_ADMIN(mGroupMeta.mSubscribeFlags))
        {
            ui->unsubscribeButton->setEnabled(true);
        }
        else
        {
            ui->unsubscribeButton->setEnabled(false);
        }
        ui->readButton->hide();
        ui->newLabel->hide();
        ui->copyLinkButton->hide();

        if (IS_MSG_NEW(mPulse.mMeta.mMsgStatus)) {
            mCloseOnRead = true;
        }
    }
    else
    {
        /* subject */
        ui->titleLabel->setText(QString::fromUtf8(mPulse.mMeta.mMsgName.c_str()));

        /* disable buttons: deletion facility not enabled with cache services yet */
        ui->clearButton->setEnabled(false);
        ui->unsubscribeButton->setEnabled(false);
        ui->clearButton->hide();
        ui->readAndClearButton->hide();
        ui->unsubscribeButton->hide();
        ui->copyLinkButton->show();

        if (IS_GROUP_SUBSCRIBED(mGroupMeta.mSubscribeFlags) || IS_GROUP_ADMIN(mGroupMeta.mSubscribeFlags))
        {
            ui->readButton->setVisible(true);

			setReadStatus(IS_MSG_NEW(mPulse.mMeta.mMsgStatus), IS_MSG_UNREAD(mPulse.mMeta.mMsgStatus) || IS_MSG_NEW(mPulse.mMeta.mMsgStatus));
        }
        else
        {
            ui->readButton->setVisible(false);
            ui->newLabel->setVisible(false);
        }

        mCloseOnRead = false;
    }

    // differences between Feed or Top of Comment.
    if (mFeedHolder)
    {
        if (mIsHome) {
            ui->commentButton->show();
        } else if (ui->commentButton->icon().isNull()){
            //Icon is seted if a comment received.
            ui->commentButton->hide();
        }

// THIS CODE IS doesn't compile - disabling until fixed.
#if 0
        if (post.mComments)
        {
            QString commentText = QString::number(post.mComments);
            commentText += " ";
            commentText += tr("Comments");
            ui->commentButton->setText(commentText);
        }
        else
        {
            ui->commentButton->setText(tr("Comment"));
        }
#endif

    }
    else
    {
        ui->commentButton->hide();
    }

    // disable voting buttons - if they have already voted.
    /*if (post.mMeta.mMsgStatus & GXS_SERV::GXS_MSG_STATUS_VOTE_MASK)
    {
        voteUpButton->setEnabled(false);
        voteDownButton->setEnabled(false);
    }*/

//	{
//		QTextDocument doc;
//		doc.setHtml( QString::fromUtf8(mPulse.mMsg.c_str()) );

//		ui->msgFrame->setVisible(doc.toPlainText().length() > 0);
//	}

//	if (wasExpanded() || ui->expandFrame->isVisible()) {
//		fillExpandFrame();
//	}
		ui->pulseMessage->setText(QString::fromUtf8(mPulse.mPulseText.c_str()));
		ui->datetimelabel->setText(DateTime::formatLongDateTime(mPulse.mRefPublishTs));

//    if ( (mPulse.mAttachmentCount != 0) || (mPulse.mSize != 0) ) {
//        ui->filelabel->setVisible(true);
//        ui->filelabel->setText(QString("(%1 %2) %3").arg(mPulse.mAttachmentCount).arg(  (mPulse.mAttachmentCount > 1)?tr("Files"):tr("File")).arg(misc::friendlyUnit(mPulse.mSize)));
//    } else {
//        ui->filelabel->setVisible(false);
//    }

//    if (mFileItems.empty() == false) {
//        std::list<SubFileItem *>::iterator it;
//        for(it = mFileItems.begin(); it != mFileItems.end(); ++it)
//        {
//            delete(*it);
//        }
//        mFileItems.clear();
//    }

//    std::list<RsGxsFile>::const_iterator it;
//    for(it = mPulse.mFiles.begin(); it != mPulse.mFiles.end(); ++it)
//    {
//        /* add file */
//        std::string path;
//        SubFileItem *fi = new SubFileItem(it->mHash, it->mName, path, it->mSize, SFI_STATE_REMOTE | SFI_TYPE_CHANNEL, RsPeerId());
//        mFileItems.push_back(fi);

//        /* check if the file is a media file */
//        if (!misc::isPreviewable(QFileInfo(QString::fromUtf8(it->mName.c_str())).suffix()))
//        {
//        fi->mediatype();
//                /* check if the file is not a media file and change text */
//        ui->playButton->setText(tr("Open"));
//        ui->playButton->setToolTip(tr("Open File"));
//        } else {
//        ui->playButton->setText(tr("Play"));
//        ui->playButton->setToolTip(tr("Play Media"));
//        }

//        QLayout *layout = ui->expandFrame->layout();
//        layout->addWidget(fi);
//    }

    mInFill = false;
}

void WireNotifyPostItem::setReadStatus(bool isNew, bool isUnread)
{
	if (isNew)
		mPulse.mMeta.mMsgStatus |= GXS_SERV::GXS_MSG_STATUS_GUI_NEW;
	else
		mPulse.mMeta.mMsgStatus &= ~GXS_SERV::GXS_MSG_STATUS_GUI_NEW;

	if (isUnread)
	{
		mPulse.mMeta.mMsgStatus |= GXS_SERV::GXS_MSG_STATUS_GUI_UNREAD;
		whileBlocking(ui->readButton)->setChecked(true);
		ui->readButton->setIcon(FilesDefs::getIconFromQtResourcePath(":/images/message-state-unread.png"));
	}
	else
	{
		mPulse.mMeta.mMsgStatus &= ~GXS_SERV::GXS_MSG_STATUS_GUI_UNREAD;
		whileBlocking(ui->readButton)->setChecked(false);
		ui->readButton->setIcon(FilesDefs::getIconFromQtResourcePath(":/images/message-state-read.png"));
	}

	//ui->newLabel->setVisible(isNew);

	//ui->feedFrame->setProperty("new", isNew);
	//ui->feedFrame->style()->unpolish(ui->feedFrame);
	//ui->feedFrame->style()->polish(  ui->feedFrame);
}

void WireNotifyPostItem::setGroup(const RsWireGroup &group)
{
	ui->groupName->setText(QString::fromStdString(group.mMeta.mGroupName));
	ui->groupName->setToolTip(QString::fromStdString(group.mMeta.mGroupName) + "@" + QString::fromStdString(group.mMeta.mAuthorId.toStdString()));

	QString title;

	title = tr("Wire Feed") + ": ";
	RetroShareLink link = RetroShareLink::createGxsGroupLink(RetroShareLink::TYPE_WIRE, group.mMeta.mGroupId, groupName());
	title += link.toHtml();
	ui->titleLabel->setText(title);

	if (group.mHeadshot.mData )
	{
		QPixmap pixmap;
		if (GxsIdDetails::loadPixmapFromData(
				group.mHeadshot.mData,
				group.mHeadshot.mSize,
				pixmap,GxsIdDetails::ORIGINAL))
		{
				ui->logoLabel->setPixmap(pixmap);
		}
	}
	else
	{
		// default.
		QPixmap pixmap = FilesDefs::getPixmapFromQtResourcePath(":/icons/wire.png");
		ui->logoLabel->setPixmap(pixmap);
	}

}

/*********** SPECIFIC FUNCTIONS ***********************/

void WireNotifyPostItem::readAndClearItem()
{
#ifdef DEBUG_ITEM
	std::cerr << "WireNotifyPostItem::readAndClearItem()";
	std::cerr << std::endl;
#endif
	readToggled(false);
	removeItem();
}

void WireNotifyPostItem::readToggled(bool /*checked*/)
{
	if (mInFill) {
		return;
	}

	mCloseOnRead = false;

	RsGxsGrpMsgIdPair msgPair = std::make_pair(groupId(), messageId());

	//rsWire->setMessageReadStatus(msgPair, isUnread());

	//setReadStatus(false, checked); // Updated by events
}

void WireNotifyPostItem::on_linkActivated(QString link)
{
	RetroShareLink rsLink(link);

	if (rsLink.valid() ) {
		QList<RetroShareLink> rsLinks;
		rsLinks.append(rsLink);
		RetroShareLink::process(rsLinks);
		removeItem();
		return;
	}
}
