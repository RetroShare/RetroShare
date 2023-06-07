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
#include <QPainter>
#include <QMessageBox>
#include <QMouseEvent>
#include <QBuffer>

#include "PulseViewGroup.h"

#include "gui/gxs/GxsIdDetails.h"
#include "gui/common/FilesDefs.h"
#include "util/DateTime.h"
#include "util/misc.h"

Q_DECLARE_METATYPE(RsWireGroup)

QColor SelectedColor = QRgb(0xff308dc7);
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
                mPixmap = pixmap;
            }
        }
        else
        {
            // Default pixmap
            QPixmap pixmap = FilesDefs::getPixmapFromQtResourcePath(":/icons/png/posted.png").scaled(400, 100);
            mPixmap = pixmap;
        }

		if (mGroup->mHeadshot.mData)
		{
			QPixmap pixmap;
			if (GxsIdDetails::loadPixmapFromData(
					mGroup->mHeadshot.mData,
					mGroup->mHeadshot.mSize,
					pixmap,GxsIdDetails::ORIGINAL))
			{
				pixmap = pixmap.scaled(50,50);
				label_headshot->setPixmap(pixmap);
			}
		}
		else
		{
            // default.
            QPixmap pixmap = FilesDefs::getPixmapFromQtResourcePath(":/icons/png/posted.png").scaled(50,50);
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

void PulseViewGroup::paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const
{
    // prepare
    painter->save();
    painter->setClipRect(option.rect);

    RsWireGroup wire = index.data(Qt::UserRole).value<RsWireGroup>();
    RsWireGroupSPtr wirePtr = std::make_shared<RsWireGroup>(wire);

    painter->fillRect( option.rect, option.palette.base().color());
    painter->restore();

//    if(mUseGrid || index.column()==0)
//    {
        // Draw a thumbnail
        PulseViewHolder *holder;
//        uint32_t flags = (mUseGrid)?(ChannelPostThumbnailView::FLAG_SHOW_TEXT | ChannelPostThumbnailView::FLAG_SCALE_FONT):0;
        PulseViewGroup w(holder,wirePtr);
        w.setBackgroundRole(QPalette::AlternateBase);
//      w.setAspectRatio(mAspectRatio);
        w.updateGeometry();
        w.adjustSize();

        QPixmap pixmap(w.size());

        if(option.state) // check if post is selected and is not empty (end of last row)
            pixmap.fill(SelectedColor);	// I dont know how to grab the backgroud color for selected objects automatically.
        else
            pixmap.fill(option.palette.base().color());

        w.render(&pixmap,QPoint(),QRegion(),QWidget::DrawChildren );// draw the widgets, not the background

        // We extract from the pixmap the part of the widget that we want. Saddly enough, Qt adds some white space
        // below the widget and there is no way to control that.

        pixmap = pixmap.copy(QRect(0,0,w.width(),w.height()));

//        if(mZoom != 1.0)
//            pixmap = pixmap.scaled(mZoom*pixmap.size(),Qt::KeepAspectRatio,Qt::SmoothTransformation);

//        if(IS_MSG_UNREAD(post.mMeta.mMsgStatus) || IS_MSG_NEW(post.mMeta.mMsgStatus))
//        {
//            QPainter p(&pixmap);
//            QFontMetricsF fm(option.font);

//            p.drawPixmap(mZoom*QPoint(0.1*fm.height(),-3.4*fm.height()),FilesDefs::getPixmapFromQtResourcePath(STAR_OVERLAY_IMAGE).scaled(mZoom*6*fm.height(),mZoom*6*fm.height(),Qt::KeepAspectRatio,Qt::SmoothTransformation));
//        }

//        if(post.mUnreadCommentCount > 0)
//        {
//            QPainter p(&pixmap);
//            QFontMetricsF fm(option.font);

//            p.drawPixmap(QPoint(pixmap.width(),0.0)+mZoom*QPoint(-2.9*fm.height(),0.4*fm.height()),
//                         FilesDefs::getPixmapFromQtResourcePath(UNREAD_COMMENT_OVERLAY_IMAGE).scaled(mZoom*3*fm.height(),mZoom*3*fm.height(),
//                                                                                              Qt::KeepAspectRatio,Qt::SmoothTransformation));
//        }
//        else if(post.mCommentCount > 0)
//        {
//            QPainter p(&pixmap);
//            QFontMetricsF fm(option.font);

//            p.drawPixmap(QPoint(pixmap.width(),0.0)+mZoom*QPoint(-2.9*fm.height(),0.4*fm.height()),
//                         FilesDefs::getPixmapFromQtResourcePath(COMMENT_OVERLAY_IMAGE).scaled(mZoom*3*fm.height(),mZoom*3*fm.height(),
//                                                                                              Qt::KeepAspectRatio,Qt::SmoothTransformation));
//        }

        painter->drawPixmap(option.rect.topLeft(),
                            pixmap.scaled(option.rect.width(),option.rect.width()*pixmap.height()/(float)pixmap.width(),
                                          Qt::IgnoreAspectRatio,Qt::SmoothTransformation));
//    }
//    else
//    {
        // We're drawing the text on the second column

        uint32_t font_height = QFontMetricsF(option.font).height();
        QPoint p = option.rect.topLeft();
        float y = p.y() + font_height;

        painter->save();

        if(option.state) // check if post is selected and is not empty (end of last row)
            painter->setPen(SelectedColor);

//        if(IS_MSG_UNREAD(post.mMeta.mMsgStatus) || IS_MSG_NEW(post.mMeta.mMsgStatus))
//        {
//            QFont font(option.font);
//            font.setBold(true);
//            painter->setFont(font);
//        }

//        {
            painter->save();

            QFont font(painter->font());
            font.setUnderline(true);
            painter->setFont(font);
            painter->restore();
//        }

        y += font_height;
        y += font_height/2.0;

//        QString info_text = QDateTime::fromMSecsSinceEpoch(qint64(1000)*post.mMeta.mPublishTs).toString(Qt::DefaultLocaleShortDate);

//        if(post.mAttachmentCount > 0)
//            info_text += ", " + QString::number(post.mAttachmentCount)+ " " +((post.mAttachmentCount>1)?tr("files"):tr("file")) + " (" + misc::friendlyUnit(qulonglong(post.mSize)) + ")" ;

//        painter->drawText(QPoint(p.x()+0.5*font_height,y),info_text);
        //y += font_height;

        painter->restore();
//    }
}

