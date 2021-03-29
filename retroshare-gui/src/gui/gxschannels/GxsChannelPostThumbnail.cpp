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

#include <math.h>

#include <QWheelEvent>
#include <QDateTime>

#include "gui/common/FilesDefs.h"
#include "gui/gxschannels/GxsChannelPostThumbnail.h"

// #define DEBUG_GXSCHANNELPOSTTHUMBNAIL 1

const float ChannelPostThumbnailView::DEFAULT_SIZE_IN_FONT_HEIGHT = 5.0;
const float ChannelPostThumbnailView::FONT_SCALE_FACTOR = 1.5;

ChannelPostThumbnailView::ChannelPostThumbnailView(const RsGxsChannelPost& post,uint32_t flags,QWidget *parent)
        : QWidget(parent),mPostTitle(nullptr),mFlags(flags), mAspectRatio(ASPECT_RATIO_2_3)
{
        // now fill the data

        init(post);
}

ChannelPostThumbnailView::ChannelPostThumbnailView(QWidget *parent,uint32_t flags)
    : QWidget(parent),mFlags(flags), mAspectRatio(ASPECT_RATIO_2_3)
{
    init(RsGxsChannelPost());
}

ChannelPostThumbnailView::~ChannelPostThumbnailView()
{
    delete mPostImage;
}

void ChannelPostThumbnailView::setText(const QString& s)
{
    if(mPostTitle == NULL)
    {
#ifdef DEBUG_GXSCHANNELPOSTTHUMBNAIL
        std::cerr << "(EE) calling setText on a ChannelPostThumbnailView without SHOW_TEXT flag!"<< std::endl;
#endif
        return;
    }

    QString ss;
    if(s.length() > 30)
        ss = s.left(30)+"...";
    else
        ss =s;

        mPostTitle->setText(ss);
}

void ChannelPostThumbnailView::setPixmap(const QPixmap& p, bool guess_aspect_ratio)
{
    mPostImage->setPicture(p);

    if(guess_aspect_ratio)// aspect ratio is automatically guessed.
    {
        // compute closest aspect ratio
        float r = p.width()/(float)p.height();

        if(r < 0.8)
            setAspectRatio(ChannelPostThumbnailView::ASPECT_RATIO_2_3);
        else if(r < 1.15)
            setAspectRatio(ChannelPostThumbnailView::ASPECT_RATIO_1_1);
        else
            setAspectRatio(ChannelPostThumbnailView::ASPECT_RATIO_16_9);
    }
}

void ChannelPostThumbnailView::setAspectRatio(AspectRatio r)
{
    mAspectRatio = r;

    QFontMetricsF fm(font());
    int W = THUMBNAIL_OVERSAMPLE_FACTOR * thumbnail_w() * fm.height() ;
    int H = THUMBNAIL_OVERSAMPLE_FACTOR * thumbnail_h() * fm.height() ;

    mPostImage->setFixedSize(W,H);
    mPostImage->reset();
    mPostImage->updateView();
}

void ChannelPostThumbnailView::init(const RsGxsChannelPost& post)
{
    QString msg = QString::fromUtf8(post.mMeta.mMsgName.c_str());
    bool is_msg_new = IS_MSG_UNREAD(post.mMeta.mMsgStatus) || IS_MSG_NEW(post.mMeta.mMsgStatus);

    QPixmap thumbnail;

    if(post.mThumbnail.mSize > 0)
        GxsIdDetails::loadPixmapFromData(post.mThumbnail.mData, post.mThumbnail.mSize, thumbnail,GxsIdDetails::ORIGINAL);
    else if(post.mMeta.mPublishTs > 0)	// this is for testing that the post is not an empty post (happens at the end of the last row)
        thumbnail = FilesDefs::getPixmapFromQtResourcePath(CHAN_DEFAULT_IMAGE);

    mPostImage = new ZoomableLabel(this);
    mPostImage->setEnableZoom(mFlags & FLAG_ALLOW_PAN);
    mPostImage->setScaledContents(true);
    mPostImage->setPicture(thumbnail);

    if(mFlags & FLAG_ALLOW_PAN)
        mPostImage->setToolTip(tr("Use mouse to center and zoom\ninto the image, so as to\n crop it for your post."));

    QVBoxLayout *layout = new QVBoxLayout(this);

    layout->addWidget(mPostImage);

    QFontMetricsF fm(font());
    int W = THUMBNAIL_OVERSAMPLE_FACTOR * thumbnail_w() * fm.height() ;
    int H = THUMBNAIL_OVERSAMPLE_FACTOR * thumbnail_h() * fm.height() ;

    mPostImage->setFixedSize(W,H);

    if(mFlags & FLAG_SHOW_TEXT)
    {
        mPostTitle = new QLabel(this);
        layout->addWidget(mPostTitle);

        QString ss = (msg.length() > 30)? (msg.left(30)+"..."):msg;

        mPostTitle->setText(ss);

        QFont font = mPostTitle->font();

        if(mFlags & ChannelPostThumbnailView::FLAG_SCALE_FONT)
            font.setPointSizeF(FONT_SCALE_FACTOR * DEFAULT_SIZE_IN_FONT_HEIGHT / 5.0 * font.pointSizeF());
        else
            font.setPointSizeF(DEFAULT_SIZE_IN_FONT_HEIGHT / 5.0 * font.pointSizeF());

        if(is_msg_new)
            font.setBold(true);

        mPostTitle->setFont(font);
        mPostTitle->setMaximumWidth(W);
        mPostTitle->setWordWrap(true);
    }
    setSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::MinimumExpanding);

    layout->addStretch();

    setLayout(layout);
    adjustSize();
    update();
}

ChannelPostThumbnailView::AspectRatio ChannelPostThumbnailView::bestAspectRatio()
{
    if(mPostImage->originalImage().isNull())
        return ASPECT_RATIO_1_1;

    float as = mPostImage->originalImage().height() / (float)mPostImage->originalImage().width() ;

    if(as <= 0.8)
        return ASPECT_RATIO_16_9;
    else if(as < 1.15)
        return ASPECT_RATIO_1_1;
    else
        return ASPECT_RATIO_2_3;
}
QSize ChannelPostThumbnailView::actualSize() const
{
    QFontMetricsF fm(font());

    if(mPostTitle != nullptr)
    {
        QMargins cm = layout()->contentsMargins();

        return QSize(width(),
                     mPostTitle->height() + mPostImage->height() + 0.5*fm.height()
                     + cm.top() + cm.bottom() + layout()->spacing());
    }
    else
        return size();
}

float ChannelPostThumbnailView::thumbnail_w() const
{
    switch(mAspectRatio)
    {
        default:
    case ASPECT_RATIO_1_1:
    case ASPECT_RATIO_UNKNOWN: return DEFAULT_SIZE_IN_FONT_HEIGHT;

    case ASPECT_RATIO_2_3:     return DEFAULT_SIZE_IN_FONT_HEIGHT;
    case ASPECT_RATIO_16_9:    return DEFAULT_SIZE_IN_FONT_HEIGHT;
    }
}
float ChannelPostThumbnailView::thumbnail_h() const
{
    switch(mAspectRatio)
    {
    default:
    case ASPECT_RATIO_1_1:
    case ASPECT_RATIO_UNKNOWN: return DEFAULT_SIZE_IN_FONT_HEIGHT;

    case ASPECT_RATIO_2_3:     return DEFAULT_SIZE_IN_FONT_HEIGHT * 3.0/2.0;
    case ASPECT_RATIO_16_9:    return DEFAULT_SIZE_IN_FONT_HEIGHT * 9.0/16.0;
    }
}

void ZoomableLabel::keyPressEvent(QKeyEvent *e)
{
    switch(e->key())
    {
    case Qt::Key_Delete:

        if(mClearEnabled)
        {
            mFullImage = QPixmap();
            emit cleared();
            e->accept();
            updateView();
        }
        break;
    default:
        QLabel::keyPressEvent(e);
    }
}

void ZoomableLabel::reset()
{
    mCenterX = mFullImage.width()/2.0;
    mCenterY = mFullImage.height()/2.0;
    mZoomFactor = 1.0/std::max(width() / (float)mFullImage.width(), height()/(float)mFullImage.height());

    updateView();
}
void ZoomableLabel::mouseMoveEvent(QMouseEvent *me)
{
    if(!mZoomEnabled)
        return;

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

    emit clicked();
}
void ZoomableLabel::mouseReleaseEvent(QMouseEvent *)
{
    mMoving = false;
}
void ZoomableLabel::wheelEvent(QWheelEvent *me)
{
    if(!mZoomEnabled)
        return;

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
#ifdef DEBUG_GXSCHANNELPOSTTHUMBNAIL
    std::cerr << "Setting new picture of size " << pix.width() << " x " << pix.height() << std::endl;
#endif
    mFullImage = pix;
    setScaledContents(true);

    reset();
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

    QRect rect(mCenterX - 0.5 * width()*mZoomFactor, mCenterY - 0.5 * height()*mZoomFactor, floor(width()*mZoomFactor), floor(height()*mZoomFactor));

#ifdef DEBUG_GXSCHANNELPOSTTHUMBNAIL
    std::cerr << "Updating view: mCenterX=" << mCenterX << ", mCenterY=" << mCenterY << ", mZoomFactor=" << mZoomFactor << std::endl;
    std::cerr << "               Image size: " << mFullImage.width() << " x " << mFullImage.height() << ", window size: " << width() << " x " << height() << std::endl;
    std::cerr << "               cropped image: " << rect.left() << "," << rect.top() << "+" << rect.width() << "+" << rect.height() << std::endl;
    std::cerr << "               saving crop to pix2.png" << std::endl;
    mFullImage.copy(rect).save("pix2.png","PNG");
#endif
    QLabel::setPixmap(mFullImage.copy(rect));
}







