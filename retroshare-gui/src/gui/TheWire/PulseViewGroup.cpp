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
#include <QTextDocumentFragment>

#include "PulseViewGroup.h"
#include "CustomFrame.h"

#include "WireGroupDialog.h"
#include "gui/gxs/GxsIdDetails.h"
#include "gui/RetroShareLink.h"
#include "gui/common/FilesDefs.h"
#include "util/DateTime.h"

#define IMAGE_WIRE           ":icons/wire.png"

Q_DECLARE_METATYPE(RsWireGroup)

/** Constructor */

PulseViewGroup::PulseViewGroup(PulseViewHolder *holder, RsWireGroupSPtr group)
:PulseViewItem(holder), mGroup(group)
{
    setupUi(this);
    setAttribute ( Qt::WA_DeleteOnClose, true );
    setup();

	connect(editButton, SIGNAL(clicked()), this, SLOT(editProfile()));

}

void PulseViewGroup::setup()
{
    if (mGroup) {
        connect(followButton, SIGNAL(clicked()), this, SLOT(actionFollow()));
        connect(toolButton_copyProfileLink, SIGNAL(clicked()), this, SLOT(actionCopyProfileLink()));

        // Twitter-like profile: name in bold, short id below (names are not unique),
        // the author id only as tooltip.
        QString groupId = QString::fromStdString(mGroup->mMeta.mGroupId.toStdString());
        QFont nameFont = label_authorName->font();
        nameFont.setBold(true);
        nameFont.setPointSizeF(nameFont.pointSizeF() * 1.6);
        label_authorName->setFont(nameFont);
        label_authorName->setTextFormat(Qt::PlainText);
        label_authorName->setText(QString::fromStdString(mGroup->mMeta.mGroupName));
        label_authorName->setToolTip(tr("Author: %1").arg(QString::fromStdString(mGroup->mMeta.mAuthorId.toStdString())));
        label_groupName->setTextFormat(Qt::PlainText);
        label_groupName->setText(WireShortId(groupId.toStdString()));
        label_groupName->setToolTip(groupId);
        label_date->setTextFormat(Qt::PlainText);
        label_date->setText(tr("Joined %1").arg(DateTime::formatDate(mGroup->mMeta.mPublishTs)));
        label_tagline->setText(QString::fromStdString(mGroup->mTagline));
        label_tagline->setVisible(!mGroup->mTagline.empty());
        label_location->setTextFormat(Qt::PlainText);
        label_location->setText(QString::fromStdString(mGroup->mLocation));
        label_location->setVisible(!mGroup->mLocation.empty());


        if (mGroup->mMasthead.mData)
        {
            QPixmap pixmap;
            if (GxsIdDetails::loadPixmapFromData(
                    mGroup->mMasthead.mData,
                    mGroup->mMasthead.mSize,
                    pixmap, GxsIdDetails::ORIGINAL))
            {
                // feed column is 600px wide: cover image fills it.
                pixmap = pixmap.scaled(600, 160, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
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
                label_headshot->setPixmap(WireRoundPixmap(pixmap, 96));
            }
        }
        else
        {
            // default.
            QPixmap pixmap = GxsIdDetails::makeDefaultGroupIcon(mGroup->mMeta.mGroupId, IMAGE_WIRE, GxsIdDetails::ORIGINAL);
            label_headshot->setPixmap(WireRoundPixmap(pixmap, 96));
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

            uint32_t following = mGroup->mGroupFollowing;
            label_extra_following->setText(BoldString(ToNumberUnits(following)));

            uint32_t followers = mGroup->mGroupFollowers;
            label_extra_followers->setText(BoldString(ToNumberUnits(followers)));
            extra_followers_Label->setText(tr("Followers (friends)"));
            extra_followers_Label->setToolTip(tr("Number of your friends who follow this Wire group.\nGXS has no global follower list."));

            // hide follow button but keep copy link visible.
            followButton->setVisible(false);
        }
        else
        {
            // hide stats.
            widget_replies->setVisible(false);
        }
    }

	setGroupSet();
	applyProfileStyle();
}

// Twitter-like profile header: no box, cover image, round avatar at the left,
// pill buttons, grey stat labels.
void PulseViewGroup::applyProfileStyle()
{
	plainFrame->setFrameShape(QFrame::NoFrame);

	frame_masthead->setMinimumSize(0, 160);
	frame_masthead->setMaximumHeight(160);

	// avatar at the bottom-left of the cover instead of centered.
	QLayoutItem *first = horizontalLayout->itemAt(0);
	if (first && first->spacerItem()) {
		delete horizontalLayout->takeAt(0);
	}
	horizontalLayout->setContentsMargins(16, 0, 0, 8);
	horizontalLayout->setAlignment(label_headshot, Qt::AlignLeft | Qt::AlignBottom);
	label_headshot->setMinimumSize(96, 96);

	line_1->hide();
	line_2->hide();

	toolButton_copyProfileLink->setText(tr("Copy link"));

	followButton->setText(tr("Follow"));

	QLabel *statLabels[] = { extra_pulses_Label, extra_replies_Label, extra_republishes_Label,
	                         extra_likes_Label, extra_following_Label, extra_followers_Label };
	for (QLabel *label : statLabels) {
		// the .ui texts carry a hardcoded html color, keep only the text (color comes from the qss).
		label->setText(label->text().isEmpty() ? QString() : QTextDocumentFragment::fromHtml(label->text()).toPlainText());
	}
	if (mGroup && (mGroup->mMeta.mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_SUBSCRIBED)) {
		extra_followers_Label->setText(tr("Followers (friends)"));
	}
	widget_replies->setMinimumHeight(0);
}

void PulseViewGroup::setGroupSet()
{
	if (mGroup->mMeta.mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_ADMIN) {
		editButton->show();
		toolButton_copyProfileLink->show();
	}
	else if (mGroup->mMeta.mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_SUBSCRIBED)
	{
		editButton->hide();
		toolButton_copyProfileLink->show();
	}
	else
	{
		editButton->hide();
		toolButton_copyProfileLink->show();
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

void PulseViewGroup::actionCopyProfileLink()
{
    std::cerr << "PulseViewGroup::actionCopyProfileLink()";
    std::cerr << std::endl;

    if (!mGroup) {
        std::cerr << "PulseViewGroup::actionCopyProfileLink() GROUP invalid";
        std::cerr << std::endl;
        return;
    }

    RetroShareLink link = RetroShareLink::createGxsGroupLink(
        RetroShareLink::TYPE_WIRE,
        mGroup->mMeta.mGroupId,
        QString::fromStdString(mGroup->mMeta.mGroupName));

    if (link.valid()) {
        QList<RetroShareLink> urls;
        urls.push_back(link);
        RSLinkClipboard::copyLinks(urls);
    }
}

void PulseViewGroup::editProfile()
{
	RsGxsGroupId groupId = mGroup->mMeta.mGroupId;
	if (groupId.isNull())
	{
		std::cerr << "PulseViewGroup::editProfile() No Group selected";
		std::cerr << std::endl;
		return;
	}

	WireGroupDialog wireEdit(GxsGroupDialog::MODE_EDIT, groupId, this);
	wireEdit.exec ();
}

