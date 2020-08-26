/*******************************************************************************
 * retroshare-gui/src/gui/gxschannels/GxsChannelPostThumbnail.cpp              *
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

#include <QWheelEvent>

#include "gui/common/FilesDefs.h"
#include "gui/gxschannels/GxsChannelPostThumbnail.h"

ChannelPostThumbnailView::ChannelPostThumbnailView(const RsGxsChannelPost& post,QWidget *parent)
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

ChannelPostThumbnailView::ChannelPostThumbnailView(QWidget *parent)
    : QWidget(parent)
{
        init(FilesDefs::getPixmapFromQtResourcePath(CHAN_DEFAULT_IMAGE), QString("New post"),false);
}

ChannelPostThumbnailView::~ChannelPostThumbnailView()
{
    delete lb;
    delete lt;
}

void ChannelPostThumbnailView::init(const QPixmap& thumbnail,const QString& msg,bool is_msg_new)
{
        QVBoxLayout *layout = new QVBoxLayout(this);

        lb = new ZoomableLabel(this);
        lb->setScaledContents(true);
        lb->setToolTip(tr("Use mouse to center and zoom into the image"));
        layout->addWidget(lb);

        lt = new QLabel(this);
        layout->addWidget(lt);

        setLayout(layout);

        setSizePolicy(QSizePolicy::Maximum,QSizePolicy::Maximum);

        QFontMetricsF fm(font());
        int W = THUMBNAIL_OVERSAMPLE_FACTOR * THUMBNAIL_W * fm.height() ;
        int H = THUMBNAIL_OVERSAMPLE_FACTOR * THUMBNAIL_H * fm.height() ;

        lb->setFixedSize(W,H);
        lb->setPicture(thumbnail);

        setText(msg);

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

void ZoomableLabel::mouseMoveEvent(QMouseEvent *me)
{
    float new_center_x = mCenterX - (me->x() - mLastX);
    float new_center_y = mCenterY - (me->y() - mLastY);

    mLastX = me->x();
    mLastY = me->y();

    if(new_center_x - 0.5 * width()*mZoomFactor < 0) return;
    if(new_center_y - 0.5 *height()*mZoomFactor < 0) return;

    if(new_center_x + 0.5 * width()*mZoomFactor >= mFullImage.width()) return;
    if(new_center_y + 0.5 *height()*mZoomFactor >=mFullImage.height()) return;

    mCenterX = new_center_x;
    mCenterY = new_center_y;

    updateView();
}
void ZoomableLabel::mousePressEvent(QMouseEvent *me)
{
    mMoving = true;
    mLastX = me->x();
    mLastY = me->y();
}
void ZoomableLabel::mouseReleaseEvent(QMouseEvent *)
{
    mMoving = false;
}
void ZoomableLabel::wheelEvent(QWheelEvent *me)
{
    float new_zoom_factor = (me->delta() > 0)?(mZoomFactor*1.05):(mZoomFactor/1.05);
    float new_center_x = mCenterX;
    float new_center_y = mCenterY;

    // Try to find centerX and centerY so that the crop does not overlap the original image

    float min_x =                       0.5 * width()*new_zoom_factor;
    float max_x =  mFullImage.width() - 0.5 * width()*new_zoom_factor;
    float min_y =                       0.5 * height()*new_zoom_factor;
    float max_y = mFullImage.height() - 0.5 * height()*new_zoom_factor;

    if(min_x >= max_x) return;
    if(min_y >= max_y) return;

    if(new_center_x < min_x) new_center_x = min_x;
    if(new_center_y < min_y) new_center_y = min_y;
    if(new_center_x > max_x) new_center_x = max_x;
    if(new_center_y > max_y) new_center_y = max_y;

    mZoomFactor = new_zoom_factor;
    mCenterX = new_center_x;
    mCenterY = new_center_y;

    updateView();
}

QPixmap ZoomableLabel::extractCroppedScaledPicture() const
{
    QRect rect(mCenterX - 0.5 * width()*mZoomFactor, mCenterY - 0.5 * height()*mZoomFactor, width()*mZoomFactor, height()*mZoomFactor);
    QPixmap pix = mFullImage.copy(rect).scaledToHeight(height(),Qt::SmoothTransformation);

    return pix;
}

void ZoomableLabel::setPicture(const QPixmap& pix)
{
    mFullImage = pix;

    mCenterX = pix.width()/2.0;
    mCenterY = pix.height()/2.0;

    updateView();
}
void ZoomableLabel::resizeEvent(QResizeEvent *e)
{
    QLabel::resizeEvent(e);

    updateView();
}

void ZoomableLabel::updateView()
{
    // The new image will be cropped from the original image, using the following rules:
    //   - first the cropped image size is computed
    //   - then center is calculated so that
    //		- the original center is preferred
    //		- if the crop overlaps the image border, the center is moved.

    QRect rect(mCenterX - 0.5 * width()*mZoomFactor, mCenterY - 0.5 * height()*mZoomFactor, width()*mZoomFactor, height()*mZoomFactor);
    QLabel::setPixmap(mFullImage.copy(rect));
}





