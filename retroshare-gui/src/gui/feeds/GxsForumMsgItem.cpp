/*******************************************************************************
 * gui/feeds/GxsForumMsgItem.cpp                                               *
 *                                                                             *
 * Copyright (c) 2014, Retroshare Team <retroshare.project@gmail.com>          *
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

#include <QTimer>
#include <QFileInfo>
#include <QStyle>

#include "rshare.h"
#include "GxsForumMsgItem.h"
#include "ui_GxsForumMsgItem.h"

#include "FeedHolder.h"
#include "gui/RetroShareLink.h"
#include "gui/common/FilesDefs.h"
#include "gui/gxs/GxsIdDetails.h"
#include "util/HandleRichText.h"
#include "util/qtthreadsutils.h"
#include "util/DateTime.h"

#include <retroshare/rsidentity.h>

#include <iostream>

/****
 * #define DEBUG_ITEM 1
 ****/

GxsForumMsgItem::GxsForumMsgItem(FeedHolder *feedHolder, uint32_t feedId, const RsGxsGroupId &groupId, const RsGxsMessageId &messageId, bool isHome, bool autoUpdate) :
    GxsFeedItem(feedHolder, feedId, groupId, messageId, isHome, rsGxsForums, autoUpdate)
{
    mLoadingStatus = LOADING_STATUS_NO_DATA;

    mLoadingGroup = false;
    mLoadingMessage = false;
    mLoadingSetAsRead = false;

    setup();
}

void GxsForumMsgItem::paintEvent(QPaintEvent *e)
{
    /* This method employs a trick to trigger a deferred loading. The post and group is requested only
     * when actually displayed on the screen. */

    if(mLoadingStatus != LOADING_STATUS_FILLED && !mGroup.mMeta.mGroupId.isNull() && !mMessage.mMeta.mMsgId.isNull())
        mLoadingStatus = LOADING_STATUS_HAS_DATA;

    if(mGroup.mMeta.mGroupId.isNull() && !mLoadingGroup)
        requestGroup();

    if(mMessage.mMeta.mMsgId.isNull() && !mLoadingMessage)
        requestMessage();

    switch(mLoadingStatus)
    {
    case LOADING_STATUS_FILLED:
    case LOADING_STATUS_NO_DATA:
    default:
        break;

    case LOADING_STATUS_HAS_DATA:
        fillGroup();
        fillMessage();

        if(!mParentMessage.mMeta.mMsgId.isNull())
            fillParentMessage();

        mLoadingStatus = LOADING_STATUS_FILLED;
        break;
    }

    GxsFeedItem::paintEvent(e) ;
}

GxsForumMsgItem::~GxsForumMsgItem()
{
    auto timeout = std::chrono::steady_clock::now() + std::chrono::milliseconds(GROUP_ITEM_LOADING_TIMEOUT_ms);

    while( (mLoadingGroup || mLoadingMessage || mLoadingSetAsRead)
           && std::chrono::steady_clock::now() < timeout)
    {
        RsDbg() << __PRETTY_FUNCTION__ << " is Waiting for "
                << (mLoadingGroup ? "Group " : "")
                << (mLoadingMessage ? "Message " : "")
                << (mLoadingSetAsRead ? "Set as read" : "")
                << "loading." << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
	delete(ui);
}

void GxsForumMsgItem::setup()
{
	/* Invoke the Qt Designer generated object setup routine */
	ui = new Ui::GxsForumMsgItem;
	ui->setupUi(this);

	setAttribute(Qt::WA_DeleteOnClose, true);

	mCloseOnRead = false;

	/* clear ui */
	ui->titleLabel->setText(tr("Loading..."));
	ui->titleLabel->setOpenExternalLinks(false); //To get linkActivated working
	connect(ui->titleLabel, SIGNAL(linkActivated(QString)), this, SLOT(on_linkActivated(QString)));
	ui->subjectLabel->clear();
	ui->subjectLabel->setOpenExternalLinks(false); //To get linkActivated working
	connect(ui->subjectLabel, SIGNAL(linkActivated(QString)), this, SLOT(on_linkActivated(QString)));
	ui->timestamplabel->clear();
	ui->parentNameLabel->clear();
	ui->currNameLabel->clear();

	/* general ones */
	connect(ui->expandButton, SIGNAL(clicked()), this, SLOT(toggle()));
	connect(ui->clearButton, SIGNAL(clicked()), this, SLOT(removeItem()));

	/* specific */
	connect(ui->readAndClearButton, SIGNAL(clicked()), this, SLOT(readAndClearItem()));
	connect(ui->unsubscribeButton, SIGNAL(clicked()), this, SLOT(unsubscribeForum()));

	ui->subjectLabel->setMinimumWidth(20);

	// hide unsubscribe button not necessary
	ui->unsubscribeButton->hide();

	ui->expandFrame->hide();
	ui->parentFrame->hide();
}

QString GxsForumMsgItem::groupName()
{
	return QString::fromUtf8(mGroup.mMeta.mGroupName.c_str());
}

void GxsForumMsgItem::loadGroup()
{
    mLoadingGroup = true;

	RsThread::async([this]()
	{
		// 1 - get group data

#ifndef DEBUG_FORUMS
        std::cerr << "Retrieving forum group data for forum " << groupId() << std::endl;
#endif

		std::vector<RsGxsForumGroup> groups;

        if(!rsGxsForums->getForumsInfo({ groupId() },groups))
		{
			RsErr() << "GxsForumGroupItem::loadGroup() ERROR getting data" << std::endl;
            mLoadingGroup = false;
            deferred_update();
            return;
		}

		if (groups.size() != 1)
		{
			std::cerr << "GxsForumGroupItem::loadGroup() Wrong number of Items";
			std::cerr << std::endl;
            mLoadingGroup = false;
            deferred_update();
            return;
		}
		RsGxsForumGroup group(groups[0]);

		RsQThreadUtils::postToObject( [group,this]()
		{
			/* Here it goes any code you want to be executed on the Qt Gui
			 * thread, for example to update the data model with new information
			 * after a blocking call to RetroShare API complete */

            mGroup = group;
            mLoadingGroup = false;

		}, this );
	});
}

void GxsForumMsgItem::loadMessage()
{
#ifndef DEBUG_ITEM
    std::cerr << "GxsForumMsgItem::loadMessage(): messageId=" << messageId() << " groupId=" << groupId() ;
	std::cerr << std::endl;
#endif
    mLoadingMessage = true;

	RsThread::async([this]()
	{
		// 1 - get group data

#ifdef DEBUG_FORUMS
		std::cerr << "Retrieving post data for post " << mThreadId << std::endl;
#endif

        auto getMessageData = [](const RsGxsGroupId& gid,const RsGxsMessageId& msg_id,RsGxsForumMsg& msg) -> bool
        {
            std::vector<RsGxsForumMsg> msgs;

            if(!rsGxsForums->getForumContent(gid,std::set<RsGxsMessageId>( { msg_id } ),msgs))
                return false;

            if (msgs.size() != 1)
                return false;

            msg = msgs[0];

            return true;
        };

        RsGxsForumMsg msg,parent_msg;

        if(!getMessageData(groupId(),messageId(),msg))
        {
            std::cerr << "GxsForumMsgItem::loadMessage() ERROR getting message data";
            mLoadingMessage = false;
            deferred_update();
            return;
        }
        // now load the parent message. If not found, it's not a problem.

        if(!msg.mMeta.mParentId.isNull() && !getMessageData(groupId(),msg.mMeta.mParentId,parent_msg))
            std::cerr << "GxsForumMsgItem::loadMessage() ERROR getting parent message data. Maybe the parent msg is not available.";

        RsQThreadUtils::postToObject( [msg,parent_msg,this]()
		{
			/* Here it goes any code you want to be executed on the Qt Gui
			 * thread, for example to update the data model with new information
			 * after a blocking call to RetroShare API complete */

            mMessage = msg;
            mParentMessage = parent_msg;
            mLoadingMessage = false;

		}, this );
	});
}

void GxsForumMsgItem::fillParentMessage()
{
    ui->parentFrame->hide();

    RetroShareLink linkParent = RetroShareLink::createGxsMessageLink(RetroShareLink::TYPE_FORUM, mParentMessage.mMeta.mGroupId, mParentMessage.mMeta.mMsgId, QString::fromUtf8(mParentMessage.mMeta.mMsgName.c_str()));
    ui->parentSubLabel->setText(linkParent.toHtml());
    ui->parentMsgLabel->setText(RsHtml().formatText(NULL, QString::fromUtf8(mParentMessage.mMsg.c_str()), RSHTML_FORMATTEXT_EMBED_SMILEYS | RSHTML_FORMATTEXT_EMBED_LINKS));
    ui->parentNameLabel->setId(mParentMessage.mMeta.mAuthorId);

    RsIdentityDetails idDetails ;
    rsIdentity->getIdDetails(mParentMessage.mMeta.mAuthorId,idDetails);

    QPixmap pixmap ;

    if(idDetails.mAvatar.mSize == 0 || !GxsIdDetails::loadPixmapFromData(idDetails.mAvatar.mData, idDetails.mAvatar.mSize, pixmap,GxsIdDetails::SMALL))
        pixmap = GxsIdDetails::makeDefaultIcon(mParentMessage.mMeta.mAuthorId,GxsIdDetails::SMALL);

    ui->parentAvatar->setPixmap(pixmap);
}
void GxsForumMsgItem::fillMessage()
{
#ifdef DEBUG_ITEM
    std::cerr << "GxsForumMsgItem::fill()";
    std::cerr << std::endl;
#endif

    if(!mIsHome && mCloseOnRead && !IS_MSG_NEW(mMessage.mMeta.mMsgStatus))
        removeItem();

    QString title = tr("Forum Feed") + ": ";
    RetroShareLink link = RetroShareLink::createGxsGroupLink(RetroShareLink::TYPE_FORUM, mMessage.mMeta.mGroupId, groupName());
    title += link.toHtml();
    ui->titleLabel->setText(title);

    setReadStatus(IS_MSG_NEW(mMessage.mMeta.mMsgStatus), IS_MSG_UNREAD(mMessage.mMeta.mMsgStatus) || IS_MSG_NEW(mMessage.mMeta.mMsgStatus));

    if (!mIsHome && IS_MSG_NEW(mMessage.mMeta.mMsgStatus))
        mCloseOnRead = true;

    RsIdentityDetails idDetails ;
    rsIdentity->getIdDetails(mMessage.mMeta.mAuthorId,idDetails);

    QPixmap pixmap ;

    if(idDetails.mAvatar.mSize == 0 || !GxsIdDetails::loadPixmapFromData(idDetails.mAvatar.mData, idDetails.mAvatar.mSize, pixmap,GxsIdDetails::SMALL))
                pixmap = GxsIdDetails::makeDefaultIcon(mMessage.mMeta.mAuthorId,GxsIdDetails::SMALL);

    ui->currAvatar->setPixmap(pixmap);

    ui->currNameLabel->setId(mMessage.mMeta.mAuthorId);

    RetroShareLink msgLink = RetroShareLink::createGxsMessageLink(RetroShareLink::TYPE_FORUM, mMessage.mMeta.mGroupId, mMessage.mMeta.mMsgId, messageName());
    ui->currSubLabel->setText(msgLink.toHtml());
    if (wasExpanded() || ui->expandFrame->isVisible())
        fillExpandFrame();

    ui->timestamplabel->setText(DateTime::formatLongDateTime(mMessage.mMeta.mPublishTs));

    /* header stuff */
    ui->subjectLabel->setText(msgLink.toHtml());

    if (mIsHome)
    {
        /* disable buttons */
        ui->clearButton->setEnabled(false);
        ui->clearButton->hide();
    }
}
void GxsForumMsgItem::fillGroup()
{
    ui->unsubscribeButton->setEnabled(IS_GROUP_SUBSCRIBED(mGroup.mMeta.mSubscribeFlags) || IS_GROUP_ADMIN(mGroup.mMeta.mSubscribeFlags)) ;

    if (IS_GROUP_PUBLISHER(mGroup.mMeta.mSubscribeFlags))
        ui->iconLabel->setPixmap(FilesDefs::getPixmapFromQtResourcePath(":/icons/png/forums.png"));
    else
        ui->iconLabel->setPixmap(FilesDefs::getPixmapFromQtResourcePath(":/icons/png/forums-default.png"));
}

void GxsForumMsgItem::fillExpandFrame()
{
	ui->currMsgLabel->setText(RsHtml().formatText(NULL, QString::fromUtf8(mMessage.mMsg.c_str()), RSHTML_FORMATTEXT_EMBED_SMILEYS | RSHTML_FORMATTEXT_EMBED_LINKS));
}

QString GxsForumMsgItem::messageName()
{
	return QString::fromUtf8(mMessage.mMeta.mMsgName.c_str());
}

void GxsForumMsgItem::doExpand(bool open)
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

		if (!mParentMessage.mMeta.mMsgId.isNull()) {
			ui->parentFrame->show();
		}

		setAsRead(true);
	}
	else
	{
		ui->expandFrame->hide();
		ui->parentFrame->hide();
        ui->expandButton->setIcon(FilesDefs::getIconFromQtResourcePath(QString(":/icons/png/down-arrow.png")));
		ui->expandButton->setToolTip(tr("Expand"));
	}

	emit sizeChanged(this);

	if (mFeedHolder)
	{
		mFeedHolder->lockLayout(this, false);
	}
}

void GxsForumMsgItem::expandFill(bool first)
{
	GxsFeedItem::expandFill(first);

	if (first) {
		fillExpandFrame();
	}
}

void GxsForumMsgItem::toggle()
{
	expand(ui->expandFrame->isHidden());
}

void GxsForumMsgItem::setReadStatus(bool isNew, bool /*isUnread*/)
{
	ui->feedFrame->setProperty("new", isNew);
	ui->feedFrame->style()->unpolish(ui->feedFrame);
	ui->feedFrame->style()->polish(  ui->feedFrame);
}

/*********** SPECIFIC FUNCTIONS ***********************/

void GxsForumMsgItem::readAndClearItem()
{
#ifdef DEBUG_ITEM
	std::cerr << "GxsForumMsgItem::readAndClearItem()";
	std::cerr << std::endl;
#endif

	setAsRead(false);
	removeItem();
}

void GxsForumMsgItem::unsubscribeForum()
{
#ifdef DEBUG_ITEM
	std::cerr << "GxsForumMsgItem::unsubscribeForum()";
	std::cerr << std::endl;
#endif

	unsubscribe();
}

void GxsForumMsgItem::setAsRead(bool doUpdate)
{
    mCloseOnRead = false;

    if(doUpdate)
        mLoadingSetAsRead = true;

    RsThread::async( [this, doUpdate]() {
        RsGxsGrpMsgIdPair msgPair = std::make_pair(groupId(), messageId());

        rsGxsForums->markRead(msgPair, true);

        if (doUpdate) {
            RsQThreadUtils::postToObject( [this]() {
                setReadStatus(false, true);
                mLoadingSetAsRead = false;
            } );
        }
    });
}

void GxsForumMsgItem::on_linkActivated(QString link)
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
