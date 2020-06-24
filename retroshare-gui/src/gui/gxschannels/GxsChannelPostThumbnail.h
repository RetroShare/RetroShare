/*******************************************************************************
 * retroshare-gui/src/gui/gxschannels/GxsChannelPostThumbnail.h                *
 *                                                                             *
 * Copyright 2020 by Retroshare Team   <retroshare.project@gmail.com>          *
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

#pragma once

#include <string>

#include <QWidget>
#include <QLabel>
#include <QLayout>

#include "retroshare/rsgxschannels.h"
#include "retroshare/rsidentity.h"

#include "gui/gxs/GxsIdDetails.h"
#include "gui/common/FilesDefs.h"

// Class to paint the thumbnails with title

class ChannelPostThumbnailView: public QWidget
{
public:
	// This variable determines the zoom factor on the text below thumbnails. 2.0 is mostly correct for all screen.
	static constexpr float THUMBNAIL_OVERSAMPLE_FACTOR = 2.0;

	// Size of thumbnails as a function of the height of the font. An aspect ratio of 3/4 is good.

	static const int THUMBNAIL_W  = 4;
	static const int THUMBNAIL_H  = 6;

    static constexpr char *CHAN_DEFAULT_IMAGE = ":images/thumb-default-video.png";

    virtual ~ChannelPostThumbnailView()
    {
     	delete lb;
     	delete lt;
    }

    ChannelPostThumbnailView(QWidget *parent=NULL): QWidget(parent)
    {
        init(FilesDefs::getPixmapFromQtResourcePath(CHAN_DEFAULT_IMAGE), QString("New post"),false);
    }

    ChannelPostThumbnailView(const RsGxsChannelPost& post,QWidget *parent=NULL)
        : QWidget(parent)
    {
        // now fill the data

		QPixmap thumbnail;

        if(post.mThumbnail.mSize > 0)
			GxsIdDetails::loadPixmapFromData(post.mThumbnail.mData, post.mThumbnail.mSize, thumbnail,GxsIdDetails::ORIGINAL);
        else if(post.mMeta.mPublishTs > 0)	// this is for testing that the post is not an empty post (happens at the end of the last row)
			thumbnail = FilesDefs::getPixmapFromQtResourcePath(CHAN_DEFAULT_IMAGE);

        init(thumbnail, QString::fromUtf8(post.mMeta.mMsgName.c_str()), IS_MSG_UNREAD(post.mMeta.mMsgStatus) || IS_MSG_NEW(post.mMeta.mMsgStatus) );

    }

	void init(const QPixmap& thumbnail,const QString& msg,bool is_msg_new)
    {
		QVBoxLayout *layout = new QVBoxLayout(this);

        lb = new QLabel(this);
        lb->setScaledContents(true);
        layout->addWidget(lb);

        lt = new QLabel(this);
        layout->addWidget(lt);

		setLayout(layout);

		setSizePolicy(QSizePolicy::Maximum,QSizePolicy::Maximum);

		QFontMetricsF fm(font());
		int W = THUMBNAIL_OVERSAMPLE_FACTOR * THUMBNAIL_W * fm.height() ;
		int H = THUMBNAIL_OVERSAMPLE_FACTOR * THUMBNAIL_H * fm.height() ;

        lb->setFixedSize(W,H);
		lb->setPixmap(thumbnail);

        lt->setText(msg);

        QFont font = lt->font();

		if(is_msg_new)
        {
            font.setBold(true);
            lt->setFont(font);
        }

        lt->setMaximumWidth(W);
        lt->setWordWrap(true);

        adjustSize();
        update();
    }

    void setPixmap(const QPixmap& p) { lb->setPixmap(p); }
    void setText(const QString& s) { lt->setText(s); }

private:
    QLabel *lb;
    QLabel *lt;
};

