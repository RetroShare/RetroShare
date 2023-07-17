/*******************************************************************************
 * gui/TheWire/PulseViewGroup.cpp                                              *
 *                                                                             *
 * Copyright (c) 2012-2020 Robert Fernie   <retroshare.project@gmail.com>      *
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

#include <QDateTime>
#include <QMessageBox>
#include <QMouseEvent>
#include <QBuffer>

#include "PulseViewGroup.h"
#include "CustomFrame.h"

#include "gui/gxs/GxsIdDetails.h"
#include "gui/common/FilesDefs.h"
#include "util/DateTime.h"

Q_DECLARE_METATYPE(RsWireGroup)

/** Constructor */

PulseViewGroup::PulseViewGroup(PulseViewHolder *holder, RsWireGroupSPtr group)
:PulseViewItem(holder), mGroup(group)
{
    setupUi(this);
    setAttribute ( Qt::WA_DeleteOnClose, true );
    setup();
}

void PulseViewGroup::setup()
{
    if (mGroup) {
        connect(followButton, SIGNAL(clicked()), this, SLOT(actionFollow()));

        label_groupName->setText("@" + QString::fromStdString(mGroup->mMeta.mGroupName));
        label_authorName->setText(BoldString(QString::fromStdString(mGroup->mMeta.mAuthorId.toStdString())));
        label_date->setText(DateTime::formatDateTime(mGroup->mMeta.mPublishTs));
        label_tagline->setText(QString::fromStdString(mGroup->mTagline));
        label_location->setText(QString::fromStdString(mGroup->mLocation));


        if (mGroup->mMasthead.mData)
        {
            QPixmap pixmap;
            if (GxsIdDetails::loadPixmapFromData(
                    mGroup->mMasthead.mData,
                    mGroup->mMasthead.mSize,
                    pixmap, GxsIdDetails::ORIGINAL))
            {
                QSize frameSize = frame_masthead->size();

                // Scale the pixmap based on the frame size
                pixmap = pixmap.scaledToWidth(frameSize.width(), Qt::SmoothTransformation);
                frame_masthead->setPixmap(pixmap);
            }
        }
// Uncomment the below code for default background
//        else
//        {
//            // Default pixmap
//            QPixmap pixmap = FilesDefs::getPixmapFromQtResourcePath(":/icons/png/posted.png");
//            QSize frameSize = frame_masthead->size();

//            // Scale the pixmap based on the frame size
//            pixmap = pixmap.scaled(frameSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
//            frame_masthead->setPixmap(pixmap);
//        }

        if (mGroup->mHeadshot.mData)
        {
            QPixmap pixmap;
            if (GxsIdDetails::loadPixmapFromData(
                    mGroup->mHeadshot.mData,
                    mGroup->mHeadshot.mSize,
                    pixmap,GxsIdDetails::ORIGINAL))
            {
                pixmap = pixmap.scaled(100,100);
                label_headshot->setPixmap(pixmap);
            }
        }
        else
        {
            // default.
            QPixmap pixmap = FilesDefs::getPixmapFromQtResourcePath(":/icons/png/posted.png").scaled(100,100);
            label_headshot->setPixmap(pixmap);
        }

        if (mGroup->mMeta.mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_SUBSCRIBED)
        {
            uint32_t pulses = mGroup->mGroupPulses + mGroup->mGroupReplies;
            uint32_t replies = mGroup->mRefReplies;
            uint32_t republishes = mGroup->mRefRepublishes;
            uint32_t likes = mGroup->mRefLikes;

            label_extra_pulses->setText(BoldString(ToNumberUnits(pulses)));
            label_extra_replies->setText(BoldString(ToNumberUnits(replies)));
            label_extra_republishes->setText(BoldString(ToNumberUnits(republishes)));
            label_extra_likes->setText(BoldString(ToNumberUnits(likes)));

            // hide follow.
            widget_actions->setVisible(false);
        }
        else
        {
            // hide stats.
            widget_replies->setVisible(false);
        }
    }
}

void PulseViewGroup::actionFollow()
{
    RsGxsGroupId groupId = mGroup->mMeta.mGroupId;
    std::cerr << "PulseViewGroup::actionFollow() following ";
    std::cerr << groupId;
    std::cerr << std::endl;

    if (mHolder) {
        mHolder->PVHfollow(groupId);
    }
}

