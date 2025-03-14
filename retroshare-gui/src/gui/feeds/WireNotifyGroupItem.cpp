/*******************************************************************************
 * gui/feeds/WireNotifyGroupItem.cpp                                                 *
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

#include "WireNotifyGroupItem.h"
#include "ui_WireNotifyGroupItem.h"

#include "FeedHolder.h"
#include "util/qtthreadsutils.h"
#include "gui/RetroShareLink.h"
#include "gui/gxs/GxsIdDetails.h"
#include "gui/common/FilesDefs.h"
#include "util/DateTime.h"

/****
 * #define DEBUG_ITEM 1
 ****/

WireNotifyGroupItem::WireNotifyGroupItem(FeedHolder *feedHolder, uint32_t feedId, const RsGxsGroupId &groupId, bool isHome, bool autoUpdate) :
    GxsGroupFeedItem(feedHolder, feedId, groupId, isHome, rsWire, autoUpdate)
{
    setup();
    requestGroup();
    addEventHandler();
}

WireNotifyGroupItem::WireNotifyGroupItem(FeedHolder *feedHolder, uint32_t feedId, const RsWireGroup &group, bool isHome, bool autoUpdate) :
    GxsGroupFeedItem(feedHolder, feedId, group.mMeta.mGroupId, isHome, rsWire, autoUpdate)
{
    setup();
    setGroup(group);
    addEventHandler();
}

void WireNotifyGroupItem::addEventHandler()
{
    mEventHandlerId = 0;
    rsEvents->registerEventsHandler( [this](std::shared_ptr<const RsEvent> event)
    {
        RsQThreadUtils::postToObject([=]()
        {
            const auto *e = dynamic_cast<const RsWireEvent*>(event.get());

            if(!e || e->mWireGroupId != this->groupId())
                return;

            switch(e->mWireEventCode)
            {
                case RsWireEventCode::FOLLOW_STATUS_CHANGED:
                case RsWireEventCode::WIRE_UPDATED:
                    break;
                default:
                    break;
            }
        }, this );
    }, mEventHandlerId, RsEventType::WIRE );
}

WireNotifyGroupItem::~WireNotifyGroupItem()
{
    rsEvents->unregisterEventsHandler(mEventHandlerId);
    delete(ui);
}

void WireNotifyGroupItem::setup()
{
    /* Invoke the Qt Designer generated object setup routine */
    ui = new(Ui::WireNotifyGroupItem);
    ui->setupUi(this);

    setAttribute(Qt::WA_DeleteOnClose, true);

    /* clear ui */
    ui->nameLabel->setText(tr("Loading..."));
    ui->titleLabel->clear();
    ui->descLabel->clear();

    /* general ones */
    connect(ui->expandButton, SIGNAL(clicked()), this, SLOT(toggle()));
    connect(ui->clearButton, SIGNAL(clicked()), this, SLOT(removeItem()));

    /* specific */
    connect(ui->subscribeButton, SIGNAL(clicked()), this, SLOT(subscribeWire()));
    connect(ui->copyLinkButton, SIGNAL(clicked()), this, SLOT(copyGroupLink()));

    //ui->copyLinkButton->hide(); // No link type at this moment
    ui->nameLabel->setEnabled(false);
    ui->expandFrame->hide();
}

bool WireNotifyGroupItem::setGroup(const RsWireGroup &group)
{
    if (groupId() != group.mMeta.mGroupId) {
        std::cerr << "WireNotifyGroupItem::setContent() - Wrong id, cannot set post";
        std::cerr << std::endl;
        return false;
    }
    mGroup = group;
    fill();

    return true;
}

void WireNotifyGroupItem::loadGroup()
{
    RsThread::async([this]()
    {
        // 1 - get group data

#ifdef DEBUG_FORUMS
        std::cerr << "Retrieving post data for post " << mThreadId << std::endl;
#endif

        std::vector<RsWireGroup> groups;
        const std::list<RsGxsGroupId> groupIds = { groupId() };

        if(!rsWire->getGroups(groupIds,groups))
        {
            RsErr() << "WireNotifyGroupItem::loadGroup() ERROR getting data" << std::endl;
            return;
        }

        if (groups.size() != 1)
        {
            std::cerr << "WireNotifyGroupItem::loadGroup() Wrong number of Items";
            std::cerr << std::endl;
            return;
        }
        RsWireGroup group(groups[0]);

        RsQThreadUtils::postToObject( [group,this]()
        {
            /* Here it goes any code you want to be executed on the Qt Gui
             * thread, for example to update the data model with new information
             * after a blocking call to RetroShare API complete */

            setGroup(group);

        }, this );
    });
}

QString WireNotifyGroupItem::groupName()
{
    return QString::fromUtf8(mGroup.mMeta.mGroupName.c_str());
}

void WireNotifyGroupItem::fill()
{
    /* fill in */

#ifdef DEBUG_ITEM
    std::cerr << "WireNotifyGroupItem::fill()";
    std::cerr << std::endl;
#endif

    // No link type at this moment
    RetroShareLink link = RetroShareLink::createGxsGroupLink(RetroShareLink::TYPE_WIRE, mGroup.mMeta.mGroupId, groupName());
    ui->nameLabel->setText(link.toHtml());
//	ui->nameLabel->setText(groupName());

    ui->descLabel->setText(QString::fromUtf8(mGroup.mTagline.c_str()));

    if (mGroup.mHeadshot.mData != NULL) {
        QPixmap wireImage;
        GxsIdDetails::loadPixmapFromData(mGroup.mHeadshot.mData, mGroup.mHeadshot.mSize, wireImage,GxsIdDetails::ORIGINAL);
        ui->logoLabel->setPixmap(QPixmap(wireImage));
    } else {
        ui->logoLabel->setPixmap(FilesDefs::getPixmapFromQtResourcePath(":/icons/wire.png"));
    }

    if(mGroup.mMeta.mLastPost==0)
        ui->infoLastPost->setText(tr("Never"));
    else
        ui->infoLastPost->setText(DateTime::formatLongDateTime(mGroup.mMeta.mLastPost));

    //TODO - nice icon for subscribed group
//	if (IS_GROUP_PUBLISHER(mGroup.mMeta.mSubscribeFlags)) {
//		ui->logoLabel->setPixmap(FilesDefs::getPixmapFromQtResourcePath(":/icons/wire.png"));
//	} else {
//		ui->logoLabel->setPixmap(FilesDefs::getPixmapFromQtResourcePath(":/icons/wire.png"));
//	}

    if (IS_GROUP_SUBSCRIBED(mGroup.mMeta.mSubscribeFlags)) {
        ui->subscribeButton->setEnabled(false);
    } else {
        ui->subscribeButton->setEnabled(true);
    }

//	if (mIsNew)
//	{
        ui->titleLabel->setText(tr("New Wire"));
//	}
//	else
//	{
//		ui->titleLabel->setText(tr("Updated Wire"));
//	}

    if (mIsHome)
    {
        /* disable buttons */
        ui->clearButton->setEnabled(false);
    }
}

void WireNotifyGroupItem::toggle()
{
    expand(ui->expandFrame->isHidden());
}

void WireNotifyGroupItem::doExpand(bool open)
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

void WireNotifyGroupItem::subscribeWire()
{
#ifdef DEBUG_ITEM
    std::cerr << "WireNotifyGroupItem::subscribeWire()";
    std::cerr << std::endl;
#endif

    subscribe();
}
