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

// Class to provide a label in which the image can be zoomed/moved. The widget size is fixed by the GUI and the user can move/zoom the image
// inside the window formed by the widget. When happy, the view-able part of the image can be extracted.

class ZoomableLabel: public QLabel
{
public:
    ZoomableLabel(QWidget *parent): QLabel(parent),mZoomFactor(1.0),mCenterX(0.0),mCenterY(0.0),mZoomEnabled(true) {}

    void setPicture(const QPixmap& pix);
    void setEnableZoom(bool b) { mZoomEnabled = b; }
    QPixmap extractCroppedScaledPicture() const;

protected:
    void mousePressEvent(QMouseEvent *ev) override;
    void mouseReleaseEvent(QMouseEvent *ev) override;
    void mouseMoveEvent(QMouseEvent *ev) override;
    void resizeEvent(QResizeEvent *ev) override;
    void wheelEvent(QWheelEvent *me) override;

    void updateView();

    QPixmap mFullImage;

    float mCenterX;
    float mCenterY;
    float mZoomFactor;
    int   mLastX,mLastY;
    bool  mMoving;
    bool  mZoomEnabled;
};

// Class to paint the thumbnails with title

class ChannelPostThumbnailView: public QWidget
{
	Q_OBJECT

public:
	// This variable determines the zoom factor on the text below thumbnails. 2.0 is mostly correct for all screen.
	static constexpr float THUMBNAIL_OVERSAMPLE_FACTOR = 2.0;

    static constexpr uint32_t FLAG_NONE      = 0x00;
    static constexpr uint32_t FLAG_SHOW_TEXT = 0x01;
    static constexpr uint32_t FLAG_ALLOW_PAN = 0x02;

	// Size of thumbnails as a function of the height of the font. An aspect ratio of 3/4 is good.

	static const int THUMBNAIL_W  = 4;
	static const int THUMBNAIL_H  = 6;

    static constexpr char *CHAN_DEFAULT_IMAGE = ":images/thumb-default-video.png";

    virtual ~ChannelPostThumbnailView();
    ChannelPostThumbnailView(QWidget *parent=NULL,uint32_t flags=FLAG_ALLOW_PAN);
    ChannelPostThumbnailView(const RsGxsChannelPost& post,uint32_t flags,QWidget *parent=NULL);

    void init(const RsGxsChannelPost& post);

    void setPixmap(const QPixmap& p) { mPostImage->setPicture(p); }
    QPixmap getCroppedScaledPicture() const { return mPostImage->extractCroppedScaledPicture() ; }

    void setText(const QString& s)
    {
        QString ss;
        if(s.length() > 30)
            ss = s.left(30)+"...";
        else
            ss =s;

        mPostTitle->setText(ss);
    }

private:
    ZoomableLabel *mPostImage;
    QLabel *mPostTitle;
    uint32_t mFlags;
};

