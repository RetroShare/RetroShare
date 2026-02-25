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

WireNotifyPostItem::WireNotifyPostItem(FeedHolder *feedHolder, uint32_t feedId, const RsGxsGroupId& groupId, const RsGxsMessageId &messageId, bool autoUpdate, const std::set<RsGxsMessageId> &older_versions) :
    GxsFeedItem(feedHolder, feedId, groupId, messageId, false, rsWire, autoUpdate)
{
    mLoadingStatus = LOADING_STATUS_NO_DATA;
    mLoadingGroup = false;
    mLoadingMessage = false;
    mLoadingComment = false;

    QVector<RsGxsMessageId> v;
    //bool self = false;

    for(std::set<RsGxsMessageId>::const_iterator it(older_versions.begin());it!=older_versions.end();++it)
        v.push_back(*it) ;

    if(older_versions.find(messageId) == older_versions.end())
        v.push_back(messageId);

    setMessageVersions(v) ;
    setup();
}

void WireNotifyPostItem::paintEvent(QPaintEvent *e)
{
    /* This method employs a trick to trigger a deferred loading. The post and group is requested only
     * when actually displayed on the screen. */

    if(mLoadingStatus != LOADING_STATUS_FILLED && !mGroupMeta.mGroupId.isNull() && !mPulse.mMeta.mMsgId.isNull() )
        mLoadingStatus = LOADING_STATUS_HAS_DATA;

    if(mGroupMeta.mGroupId.isNull() && !mLoadingGroup)
        requestGroup();

    if(mPulse.mMeta.mMsgId.isNull() && !mLoadingMessage)
        requestMessage();

    switch(mLoadingStatus)
    {
    case LOADING_STATUS_FILLED:
    case LOADING_STATUS_NO_DATA:
    default:
        break;

    case LOADING_STATUS_HAS_DATA:
        fill();
        setGroup(mGroup);
        mLoadingStatus = LOADING_STATUS_FILLED;
        break;
    }

    GxsFeedItem::paintEvent(e) ;
}

WireNotifyPostItem::~WireNotifyPostItem()
{
    auto timeout = std::chrono::steady_clock::now() + std::chrono::milliseconds(GROUP_ITEM_LOADING_TIMEOUT_ms);

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
    //ui->editButton->setIcon(FilesDefs::getIconFromQtResourcePath(":/icons/png/pencil-edit-button.png"));
//    ui->copyLinkButton->setIcon(FilesDefs::getIconFromQtResourcePath(":/icons/png/copy.png"));
//    ui->expandButton->setIcon(FilesDefs::getIconFromQtResourcePath(":/icons/png/down-arrow.png"));
//    ui->readAndClearButton->setIcon(FilesDefs::getIconFromQtResourcePath(":/icons/png/correct.png"));
//    ui->clearButton->setIcon(FilesDefs::getIconFromQtResourcePath(":/icons/png/exit2.png"));

    setAttribute(Qt::WA_DeleteOnClose, true);

    mInFill = false;
    mCloseOnRead = false;

    /* clear ui */
    ui->titleLabel->setText(tr("Loading..."));
	ui->titleLabel->setOpenExternalLinks(false); //To get linkActivated working
	connect(ui->titleLabel, SIGNAL(linkActivated(QString)), this, SLOT(on_linkActivated(QString)));
    ui->datetimelabel->clear();
//    ui->newCommentLabel->hide();
//    ui->commLabel->hide();

    /* general ones */
    connect(ui->expandButton, SIGNAL(clicked()), this, SLOT(toggle()));
    connect(ui->clearButton, SIGNAL(clicked()), this, SLOT(removeItem()));

    /* specific */
    connect(ui->readAndClearButton, SIGNAL(clicked()), this, SLOT(readAndClearItem()));
    connect(ui->unsubscribeButton, SIGNAL(clicked()), this, SLOT(unsubscribe()));

    //connect(ui->editButton, SIGNAL(clicked()), this, SLOT(edit(void)));
    connect(ui->copyLinkButton, SIGNAL(clicked()), this, SLOT(copyMessageLink()));

    connect(ui->readButton, SIGNAL(toggled(bool)), this, SLOT(readToggled(bool)));

    // hide unsubscribe button not necessary
    ui->unsubscribeButton->hide();
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
	if (!mPulse.mMeta.mMsgName.empty())
		return QString::fromUtf8(mPulse.mMeta.mMsgName.c_str());

	return tr("New Pulse");
}

void WireNotifyPostItem::loadMessage()
{
#ifdef DEBUG_ITEM
    std::cerr << "WireNotifyPostItem::loadMessage()";
    std::cerr << std::endl;
#endif
    mLoadingMessage = true;

    RsGxsGroupId grpId = groupId();
    RsGxsMessageId msgId = messageId();

    RsThread::async([this, grpId, msgId]()
    {
        RsWirePulseSPtr pPulse;

        if (!rsWire->getWirePulse(grpId, msgId, pPulse))
        {
            RsErr() << "WireNotifyPostItem::loadMessage() ERROR getting pulse" << std::endl;
            mLoadingMessage = false;
            deferred_update();
            return;
        }

        if (pPulse)
        {
#ifdef DEBUG_ITEM
            std::cerr << (void*)this << ": Obtained post, with msgId = " << pPulse->mMeta.mMsgId << std::endl;
#endif
            const RsWirePulse pulse(*pPulse);

            RsQThreadUtils::postToObject([pulse, this]()
            {
                mPulse = pulse;
                mLoadingMessage = false;
                update();	// triggers paintEvent if needed
            }, this);
        }
        else
        {
            RsQThreadUtils::postToObject([this]()
            {
                removeItem();
                mLoadingMessage = false;
                update();
            }, this);
        }
    });
}

void WireNotifyPostItem::loadComment()
{
    // TODO: implement Wire comment loading when Wire comments are supported
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
            mLoadingGroup = false;
            deferred_update();
            return;
        }

        if (groups.size() != 1)
        {
            std::cerr << "WireNotifyPostItem::loadGroup() Wrong number of Items";
            std::cerr << std::endl;
            mLoadingGroup = false;
            deferred_update();
            return;
        }
        RsWireGroup group = groups[0];

        RsQThreadUtils::postToObject( [group,this]()
        {
            /* Here it goes any code you want to be executed on the Qt Gui
             * thread, for example to update the data model with new information
             * after a blocking call to RetroShare API complete */

            mGroup = group;
            mGroupMeta = group.mMeta;
            mLoadingGroup = false;

            update();	// triggers paintEvent if needed

        }, this );
    });
}

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

    mInFill = true;

    QString title;
    QString msgText;

    // Determine pulse type for display
    QString pulseTypeStr;
    if (mPulse.mPulseType & WIRE_PULSE_TYPE_LIKE) {
        pulseTypeStr = tr("Like");
    } else if (mPulse.mPulseType & WIRE_PULSE_TYPE_REPUBLISH) {
        pulseTypeStr = tr("Republish");
    } else if (mPulse.mPulseType & WIRE_PULSE_TYPE_REPLY) {
        pulseTypeStr = tr("Reply");
    } else {
        pulseTypeStr = tr("Pulse");
    }

    if (mCloseOnRead && !IS_MSG_NEW(mPulse.mMeta.mMsgStatus)) {
        removeItem();
    }

    msgText = pulseTypeStr + ": ";
    RetroShareLink msgLink = RetroShareLink::createGxsMessageLink(RetroShareLink::TYPE_WIRE, mPulse.mMeta.mGroupId, mPulse.mMeta.mMsgId, messageName());
    msgText += msgLink.toHtml();
    ui->subjectLabel->setText(msgText);

    ui->pulseMessage->setText(QString::fromUtf8(mPulse.mPulseText.c_str()));
    ui->datetimelabel->setText(DateTime::formatLongDateTime(mPulse.mRefPublishTs));

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
	ui->groupName->hide();

	QString title;

	// Determine pulse type for title
	QString pulseTypeStr;
	if (mPulse.mPulseType & WIRE_PULSE_TYPE_LIKE) {
		pulseTypeStr = tr("New Like");
	} else if (mPulse.mPulseType & WIRE_PULSE_TYPE_REPUBLISH) {
		pulseTypeStr = tr("New Republish");
	} else if (mPulse.mPulseType & WIRE_PULSE_TYPE_REPLY) {
		pulseTypeStr = tr("New Reply");
	} else {
		pulseTypeStr = tr("Wire Feed");
	}

	title = pulseTypeStr + ": ";
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
