/*
 * Retroshare Gxs Support
 *
 * Copyright 2012-2013 by Robert Fernie.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 *
 * Please report all bugs and problems to "retroshare@lunamutt.com".
 *
 */

#include <QApplication>
#include <QThread>
#include <QTimerEvent>
#include <QMutexLocker>

#include <math.h>
#include "GxsIdDetails.h"

#include <retroshare/rspeers.h>

#include <iostream>
#include <QPainter>

/* Images for tag icons */
#define IMAGE_LOADING     ":/images/folder-draft.png"
#define IMAGE_PGPKNOWN    ":/images/tags/pgp-known.png"
#define IMAGE_PGPUNKNOWN  ":/images/tags/pgp-unknown.png"
#define IMAGE_ANON        ":/images/tags/anon.png"

#define IMAGE_DEV_AMBASSADOR     ":/images/tags/dev-ambassador.png"
#define IMAGE_DEV_CONTRIBUTOR    ":/images/tags/vote_down.png"
#define IMAGE_DEV_TRANSLATOR     ":/images/tags/dev-translator.png"
#define IMAGE_DEV_PATCHER        ":/images/tags/dev-patcher.png"
#define IMAGE_DEV_DEVELOPER      ":/images/tags/developer.png"

#define TIMER_INTERVAL 1000
#define MAX_ATTEMPTS   10

static const int IconSize = 20;

const int kRecognTagClass_DEVELOPMENT = 1;

const int kRecognTagType_Dev_Ambassador 	= 1;
const int kRecognTagType_Dev_Contributor 	= 2;
const int kRecognTagType_Dev_Translator 	= 3;
const int kRecognTagType_Dev_Patcher     	= 4;
const int kRecognTagType_Dev_Developer	 	= 5;

/* The global object */
GxsIdDetails *GxsIdDetails::mInstance = NULL ;

GxsIdDetails::GxsIdDetails()
    : QObject()
{
	mCheckTimerId = 0;

	connect(this, SIGNAL(startTimerFromThread()), this, SLOT(doStartTimer()));
}

GxsIdDetails::~GxsIdDetails()
{
}

void GxsIdDetails::objectDestroyed(QObject *object)
{
	if (!object) {
		return;
	}

	QMutexLocker lock(&mMutex);

	/* Object is about to be destroyed, remove it from pending list */
	QList<CallbackData>::iterator dataIt;
	for (dataIt = mPendingData.begin(); dataIt != mPendingData.end(); ) {
		CallbackData &pendingData = *dataIt;

		if (pendingData.mObject == object) {
			dataIt = mPendingData.erase(dataIt);
			continue;
		}

		++dataIt;
	}
}

void GxsIdDetails::connectObject_locked(QObject *object, bool doConnect)
{
	if (!object) {
		return;
	}

	/* Search Object in pending list */
	QList<CallbackData>::iterator dataIt;
	for (dataIt = mPendingData.begin(); dataIt != mPendingData.end(); ++dataIt) {
		if (dataIt->mObject == object) {
			/* Object still/already in pending list */
			return;
		}
	}

	if (doConnect) {
		connect(object, SIGNAL(destroyed(QObject*)), this, SLOT(objectDestroyed(QObject*)));
	} else {
		disconnect(object, SIGNAL(destroyed(QObject*)), this, SLOT(objectDestroyed(QObject*)));
	}
}

void GxsIdDetails::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == mCheckTimerId) {
		/* Stop timer */
		killTimer(mCheckTimerId);
		mCheckTimerId = 0;

		if (rsIdentity) {
			QMutexLocker lock(&mMutex);

			if (!mPendingData.empty()) {
				/* Check pending id's */
				QList<CallbackData>::iterator dataIt;
				for (dataIt = mPendingData.begin(); dataIt != mPendingData.end(); ) {
					CallbackData &pendingData = *dataIt;

					RsIdentityDetails details;
					if (rsIdentity->getIdDetails(pendingData.mId, details)) {
						/* Got details */
						pendingData.mCallback(GXS_ID_DETAILS_TYPE_DONE, details, pendingData.mObject, pendingData.mData);

						QObject *object = pendingData.mObject;
						dataIt = mPendingData.erase(dataIt);
						connectObject_locked(object, false);

						continue;
					}

					if (++pendingData.mAttempt > MAX_ATTEMPTS) {
						/* Max attempts reached, stop trying */
						details.mId = pendingData.mId;
						pendingData.mCallback(GXS_ID_DETAILS_TYPE_FAILED, details, pendingData.mObject, pendingData.mData);

						QObject *object = pendingData.mObject;
						dataIt = mPendingData.erase(dataIt);
						connectObject_locked(object, false);

						continue;
					}

					++dataIt;
				}
			}
		}

		QMutexLocker lock(&mMutex);

		if (mPendingData.empty()) {
			/* All done */
			mInstance = NULL;
			deleteLater();
		} else {
			/* Start timer */
			doStartTimer();
		}
	}

	QObject::timerEvent(event);
}

void GxsIdDetails::doStartTimer()
{
	if (mCheckTimerId) {
		/* Timer is running */
		return;
	}

	mCheckTimerId = startTimer(TIMER_INTERVAL);
}

bool GxsIdDetails::process(const RsGxsId &id, GxsIdDetailsCallbackFunction callback, QObject *object, const QVariant &data)
{
	if (!callback) {
		return false;
	}

	RsIdentityDetails details;
	if (id.isNull()) {
		callback(GXS_ID_DETAILS_TYPE_EMPTY, details, object, data);
		return true;
	}

	/* Try to get the information */
	if (rsIdentity && rsIdentity->getIdDetails(id, details)) {
		callback(GXS_ID_DETAILS_TYPE_DONE, details, object, data);
		return true;
	}

	details.mId = id;

	/* Add id to the pending list */
	if (!mInstance) {
		mInstance = new GxsIdDetails;
		mInstance->moveToThread(qApp->thread());
	}
    callback(GXS_ID_DETAILS_TYPE_LOADING, details, object, data);

	CallbackData pendingData;
	pendingData.mId = id;
	pendingData.mCallback = callback;
	pendingData.mObject = object;
	pendingData.mData = data;

	{
		QMutexLocker lock(&mInstance->mMutex);

		/* Connect signal "destroy" */
		mInstance->connectObject_locked(object, true);

		mInstance->mPendingData.push_back(pendingData);
	}

	/* Start timer */
	if (QThread::currentThread() == qApp->thread()) {
		/* Start timer directly */
		mInstance->doStartTimer();
	} else {
		/* Send signal to start timer in main thread */
		emit mInstance->startTimerFromThread();
	}

	return true;
}

static bool findTagIcon(int tag_class, int /*tag_type*/, QIcon &icon)
{
	switch(tag_class)
	{
		default:
		case 0:
			icon = QIcon(IMAGE_DEV_AMBASSADOR);
			break;
		case 1:
			icon = QIcon(IMAGE_DEV_CONTRIBUTOR);
			break;
	}
	return true;
}

//QImage GxsIdDetails::makeDefaultIcon(const RsGxsId& id)
//{
//	static std::map<RsGxsId,QImage> image_cache ;
//
//	std::map<RsGxsId,QImage>::const_iterator it = image_cache.find(id) ;
//
//	if(it != image_cache.end())
//		return it->second ;
//
//	int S = 128 ;
//	QImage pix(S,S,QImage::Format_RGB32) ;
//
//	uint64_t n = reinterpret_cast<const uint64_t*>(id.toByteArray())[0] ;
//
//	uint8_t a[8] ;
//	for(int i=0;i<8;++i)
//	{
//		a[i] = n&0xff ;
//		n >>= 8 ;
//	}
//	QColor val[16] = {
//	    QColor::fromRgb( 255, 110, 180),
//	    QColor::fromRgb( 238,  92,  66),
//	    QColor::fromRgb( 255, 127,  36),
//	    QColor::fromRgb( 255, 193, 193),
//	    QColor::fromRgb( 127, 255, 212),
//	    QColor::fromRgb(   0, 255, 255),
//	    QColor::fromRgb( 224, 255, 255),
//	    QColor::fromRgb( 199,  21, 133),
//	    QColor::fromRgb(  50, 205,  50),
//	    QColor::fromRgb( 107, 142,  35),
//	    QColor::fromRgb(  30, 144, 255),
//	    QColor::fromRgb(  95, 158, 160),
//	    QColor::fromRgb( 143, 188, 143),
//	    QColor::fromRgb( 233, 150, 122),
//	    QColor::fromRgb( 151, 255, 255),
//	    QColor::fromRgb( 162, 205,  90),
//	};
//
//	int c1 = (a[0]^a[1]) & 0xf ;
//	int c2 = (a[1]^a[2]) & 0xf ;
//	int c3 = (a[2]^a[3]) & 0xf ;
//	int c4 = (a[3]^a[4]) & 0xf ;
//
//	for(int i=0;i<S/2;++i)
//		for(int j=0;j<S/2;++j)
//		{
//			float res1 = 0.0f ;
//			float res2 = 0.0f ;
//			float f = 1.70;
//
//			for(int k1=0;k1<4;++k1)
//				for(int k2=0;k2<4;++k2)
//				{
//					res1 += cos( (2*M_PI*i/(float)S) * k1 * f) * (a[k1  ] & 0xf) + sin( (2*M_PI*j/(float)S) * k2 * f) * (a[k2  ] >> 4) + sin( (2*M_PI*i/(float)S) * k1 * f) * cos( (2*M_PI*j/(float)S) * k2 * f) * (a[k1+k2] >> 4) ;
//					res2 += cos( (2*M_PI*i/(float)S) * k2 * f) * (a[k1+2] & 0xf) + sin( (2*M_PI*j/(float)S) * k1 * f) * (a[k2+1] >> 4) + sin( (2*M_PI*i/(float)S) * k2 * f) * cos( (2*M_PI*j/(float)S) * k1 * f) * (a[k1^k2] >> 4) ;
//				}
//
//			uint32_t q = 0 ;
//			if(res1 >= 0.0f) q += val[c1].rgb() ; else q += val[c2].rgb() ;
//			if(res2 >= 0.0f) q += val[c3].rgb() ; else q += val[c4].rgb() ;
//
//			pix.setPixel( i, j, q) ;
//			pix.setPixel( S-1-i, j, q) ;
//			pix.setPixel( S-1-i, S-1-j, q) ;
//			pix.setPixel(     i, S-1-j, q) ;
//		}
//
//	image_cache[id] = pix.scaled(128,128,Qt::KeepAspectRatio,Qt::SmoothTransformation) ;
//
//	return image_cache[id] ;
//}

/**
 * @brief GxsIdDetails::makeIdentIcon
 * @param id: RsGxsId to compute
 * @return QImage representing an IndentIcon cf:
 * http://en.wikipedia.org/wiki/Identicon
 * Bring the source code from this adaptation:
 * http://francisshanahan.com/identicon5/test.html
 */
QImage GxsIdDetails::makeDefaultIconLocal_locked(const RsGxsId& id)
{
    QMutexLocker lock(&mMutex2);

    std::map<RsGxsId,QImage>::const_iterator it = image_cache.find(id) ;

    if(it != image_cache.end())
		return it->second ;

    image_cache[id] = drawIdentIcon(QString::fromStdString(id.toStdString()),64*3, true);

    return image_cache[id] ;
}
QImage GxsIdDetails::makeDefaultIcon(const RsGxsId& id)
{
//    if(mInstance != NULL)
//        return mInstance->makeDefaultIconLocal_locked(id) ;
 //   else
        return  drawIdentIcon(QString::fromStdString(id.toStdString()),64*3, true);
}

/**
 * @brief GxsIdDetails::getSprite
 * @param shapeType: type of shape (0 to 15)
 * @param size: Size for shape
 * @return QList<qreal> for all point for path drawing (shape)
 */
QList<qreal> GxsIdDetails::getSprite(quint8 shapeType, quint16 size)
	{
	QList<qreal> sprite;
	switch (shapeType) {
		case 0: // Triangle
			sprite.append(0.5);sprite.append(1.0);
			sprite.append(1.0);sprite.append(0.0);
			sprite.append(1.0);sprite.append(1.0);
		break;
		case 1: // Parallelogram
			sprite.append(0.5);sprite.append(0.0);
			sprite.append(1.0);sprite.append(0.0);
			sprite.append(0.5);sprite.append(1.0);
			sprite.append(0.0);sprite.append(1.0);
		break;
		case 2: // mouse ears
			sprite.append(0.5);sprite.append(0);
			sprite.append(1);sprite.append(0);
			sprite.append(1);sprite.append(1);
			sprite.append(0.5);sprite.append(1);
			sprite.append(1);sprite.append(0.5);
		break;
		case 3: // Ribbon
			sprite.append(0);sprite.append(0.5);
			sprite.append(0.5);sprite.append(0);
			sprite.append(1);sprite.append(0.5);
			sprite.append(0.5);sprite.append(1);
			sprite.append(0.5);sprite.append(0.5);
		break;
		case 4: // Sails
			sprite.append(0);sprite.append(0.5);
			sprite.append(1);sprite.append(0);
			sprite.append(1);sprite.append(1);
			sprite.append(0);sprite.append(1);
			sprite.append(1);sprite.append(0.5);
		break;
		case 5: // Fins
			sprite.append(1);sprite.append(0);
			sprite.append(1);sprite.append(1);
			sprite.append(0.5);sprite.append(1);
			sprite.append(1);sprite.append(0.5);
			sprite.append(0.5);sprite.append(0.5);
		break;
		case 6: // Beak
			sprite.append(0);sprite.append(0);
			sprite.append(1);sprite.append(0);
			sprite.append(1);sprite.append(0.5);
			sprite.append(0);sprite.append(0);
			sprite.append(0.5);sprite.append(1);
			sprite.append(0);sprite.append(1);
		break;
		case 7: // Chevron
			sprite.append(0);sprite.append(0);
			sprite.append(0.5);sprite.append(0);
			sprite.append(1);sprite.append(0.5);
			sprite.append(0.5);sprite.append(1);
			sprite.append(0);sprite.append(1);
			sprite.append(0.5);sprite.append(0.5);
		break;
		case 8: // Fish
			sprite.append(0.5);sprite.append(0);
			sprite.append(0.5);sprite.append(0.5);
			sprite.append(1);sprite.append(0.5);
			sprite.append(1);sprite.append(1);
			sprite.append(0.5);sprite.append(1);
			sprite.append(0.5);sprite.append(0.5);
			sprite.append(0);sprite.append(0.5);
		break;
		case 9: // Kite
			sprite.append(0);sprite.append(0);
			sprite.append(1);sprite.append(0);
			sprite.append(0.5);sprite.append(0.5);
			sprite.append(1);sprite.append(0.5);
			sprite.append(0.5);sprite.append(1);
			sprite.append(0.5);sprite.append(0.5);
			sprite.append(0);sprite.append(1);
		break;
		case 10: // Trough
			sprite.append(0);sprite.append(0.5);
			sprite.append(0.5);sprite.append(1);
			sprite.append(1);sprite.append(0.5);
			sprite.append(0.5);sprite.append(0);
			sprite.append(1);sprite.append(0);
			sprite.append(1);sprite.append(1);
			sprite.append(0);sprite.append(1);
		break;
		case 11: // Rays
			sprite.append(0.5);sprite.append(0);
			sprite.append(1);sprite.append(0);
			sprite.append(1);sprite.append(1);
			sprite.append(0.5);sprite.append(1);
			sprite.append(1);sprite.append(0.75);
			sprite.append(0.5);sprite.append(0.5);
			sprite.append(1);sprite.append(0.25);
		break;
		case 12: // Double rhombus
			sprite.append(0);sprite.append(0.5);
			sprite.append(0.5);sprite.append(0);
			sprite.append(0.5);sprite.append(0.5);
			sprite.append(1);sprite.append(0);
			sprite.append(1);sprite.append(0.5);
			sprite.append(0.5);sprite.append(1);
			sprite.append(0.5);sprite.append(0.5);
			sprite.append(0);sprite.append(1);
		break;
		case 13: // Crown
			sprite.append(0);sprite.append(0);
			sprite.append(1);sprite.append(0);
			sprite.append(1);sprite.append(1);
			sprite.append(0);sprite.append(1);
			sprite.append(1);sprite.append(0.5);
			sprite.append(0.5);sprite.append(0.25);
			sprite.append(0.5);sprite.append(0.75);
			sprite.append(0);sprite.append(0.5);
			sprite.append(0.5);sprite.append(0.25);
		break;
		case 14: // Radioactive
			sprite.append(0);sprite.append(0.5);
			sprite.append(0.5);sprite.append(0.5);
			sprite.append(0.5);sprite.append(0);
			sprite.append(1);sprite.append(0);
			sprite.append(0.5);sprite.append(0.5);
			sprite.append(1);sprite.append(0.5);
			sprite.append(0.5);sprite.append(1);
			sprite.append(0.5);sprite.append(0.5);
			sprite.append(0);sprite.append(1);
		break;
		default: // Tiles
			sprite.append(0);sprite.append(0);
			sprite.append(1);sprite.append(0);
			sprite.append(0.5);sprite.append(0.5);
			sprite.append(0.5);sprite.append(0);
			sprite.append(0);sprite.append(0.5);
			sprite.append(1);sprite.append(0.5);
			sprite.append(0.5);sprite.append(1);
			sprite.append(0.5);sprite.append(0.5);
			sprite.append(0);sprite.append(1);
		break;
		}

	// Scale up
	for (quint8 i = 0; i < sprite.size(); ++i) {
		sprite[i] = sprite[i] * size;
	}

	return sprite;
}

/**
 * @brief GxsIdDetails::getCenter
 * @param shapeType: type of shape (0 to 15)
 * @param size: Size for shape
 * @return QList<qreal> for all point for path drawing (shape) in center
 */
QList<qreal> GxsIdDetails::getCenter(quint8 shapeType, quint16 size)
{
	QList<qreal> sprite;
	switch (shapeType) {
		case 0: // Empty
			sprite.clear();
		break;
		case 1: // Fill
			sprite.append(0);sprite.append(0);
			sprite.append(1);sprite.append(0);
			sprite.append(1);sprite.append(1);
			sprite.append(0);sprite.append(1);
		break;
		case 2: // Diamond
			sprite.append(0.5);sprite.append(0);
			sprite.append(1);sprite.append(0.5);
			sprite.append(0.5);sprite.append(1);
			sprite.append(0);sprite.append(0.5);
		break;
		case 3: // Reverse diamond
			sprite.append(0);sprite.append(0);
			sprite.append(1);sprite.append(0);
			sprite.append(1);sprite.append(1);
			sprite.append(0);sprite.append(1);
			sprite.append(0);sprite.append(0.5);
			sprite.append(0.5);sprite.append(1);
			sprite.append(1);sprite.append(0.5);
			sprite.append(0.5);sprite.append(0);
			sprite.append(0);sprite.append(0.5);
		break;
		case 4: // Cross
			sprite.append(0.25);sprite.append(0);
			sprite.append(0.75);sprite.append(0);
			sprite.append(0.5);sprite.append(0.5);
			sprite.append(1);sprite.append(0.25);
			sprite.append(1);sprite.append(0.75);
			sprite.append(0.5);sprite.append(0.5);
			sprite.append(0.75);sprite.append(1);
			sprite.append(0.25);sprite.append(1);
			sprite.append(0.5);sprite.append(0.5);
			sprite.append(0);sprite.append(0.75);
			sprite.append(0);sprite.append(0.25);
			sprite.append(0.5);sprite.append(0.5);
		break;
		case 5: // Morning star
			sprite.append(0);sprite.append(0);
			sprite.append(0.5);sprite.append(0.25);
			sprite.append(1);sprite.append(0);
			sprite.append(0.75);sprite.append(0.5);
			sprite.append(1);sprite.append(1);
			sprite.append(0.5);sprite.append(0.75);
			sprite.append(0);sprite.append(1);
			sprite.append(0.25);sprite.append(0.5);
		break;
		case 6: // Small square
			sprite.append(0.33);sprite.append(0.33);
			sprite.append(0.67);sprite.append(0.33);
			sprite.append(0.67);sprite.append(0.67);
			sprite.append(0.33);sprite.append(0.67);
		break;
		case 7: // Checkerboard
			sprite.append(0);sprite.append(0);
			sprite.append(0.33);sprite.append(0);
			sprite.append(0.33);sprite.append(0.33);
			sprite.append(0.66);sprite.append(0.33);
			sprite.append(0.67);sprite.append(0);
			sprite.append(1);sprite.append(0);
			sprite.append(1);sprite.append(0.33);
			sprite.append(0.67);sprite.append(0.33);
			sprite.append(0.67);sprite.append(0.67);
			sprite.append(1);sprite.append(0.67);
			sprite.append(1);sprite.append(1);
			sprite.append(0.67);sprite.append(1);
			sprite.append(0.67);sprite.append(0.67);
			sprite.append(0.33);sprite.append(0.67);
			sprite.append(0.33);sprite.append(1);
			sprite.append(0);sprite.append(1);
			sprite.append(0);sprite.append(0.67);
			sprite.append(0.33);sprite.append(0.67);
			sprite.append(0.33);sprite.append(0.33);
			sprite.append(0);sprite.append(0.33);
		break;
		default: // Tiles
			sprite.append(0);sprite.append(0);
			sprite.append(1);sprite.append(0);
			sprite.append(0.5);sprite.append(0.5);
			sprite.append(0.5);sprite.append(0);
			sprite.append(0);sprite.append(0.5);
			sprite.append(1);sprite.append(0.5);
			sprite.append(0.5);sprite.append(1);
			sprite.append(0.5);sprite.append(0.5);
			sprite.append(0);sprite.append(1);
		break;
	}
	/* apply ratios */
	for (quint8 i = 0; i < sprite.size(); ++i) {
		sprite[i] = sprite[i] * size;
	}

	return sprite;
}

/**
 * @brief GxsIdDetails::drawRotatedPolygon
 * @param pixmap: The pixmap write to draw
 * @param sprite: path to follow
 * @param x: Offset to start
 * @param y: Offset to start
 * @param shapeangle: Angle of shape to draw
 * @param angle: Angle (where 3x3 in picture)
 * @param size: Size of shape
 * @param fillColor: Color to fill shape
 */
void GxsIdDetails::drawRotatedPolygon( QPixmap *pixmap,
                                       QList<qreal> sprite,
                                       quint16 x, quint16 y,
                                       qreal shapeangle, qreal angle,
                                       quint16 size, QColor fillColor)
{
	qreal halfSize = size / 2;
	QPainter painter (pixmap);
	painter.save();
	painter.setBrush(fillColor);
	painter.setPen(fillColor);

	painter.translate(x, y);
	painter.rotate(angle);
	painter.save();
	painter.translate(halfSize, halfSize);
	QList<qreal> tmpSprite;
	for (quint8 p = 0; p < sprite.size(); ++p) {
		tmpSprite.append(sprite[p] - halfSize);
	}
	painter.rotate(shapeangle);
	if (tmpSprite.size() >= 2) {
		QPainterPath path;
		path.setFillRule(Qt::WindingFill);
		path.moveTo(tmpSprite[0], tmpSprite[1]);
		for (int i = 2; i < tmpSprite.size(); ++++i) {
			path.lineTo(tmpSprite[i], tmpSprite[i + 1]);
		}
		//path.lineTo(tmpSprite[0], tmpSprite[1]);
		painter.drawPath(path);
	}
	// Black outline for debugging
	//painter.drawRect(-halfSize, -halfSize, size, size);
	painter.restore();
	painter.restore();
}

/**
 * @brief GxsIdDetails::drawIdentIcon
 * @param hash: Hash to compute (min 20 char)
 * @param width: Size of picture to get
 * @param rotate: If the shapes could be rotated
 * @return QImage of computed hash
 */
QImage GxsIdDetails::drawIdentIcon( QString hash, quint16 width, bool rotate)
{
	bool ok;
	quint8 csh = hash.mid(0, 1).toInt(&ok,16);// Corner sprite shape
	quint8 ssh = hash.mid(1, 1).toInt(&ok, 16); // Side sprite shape
	quint8 xsh = hash.mid(2, 1).toInt(&ok, 16) & 7; // Center sprite shape

	qreal halfPi = 90;// M_PI/2;
	qreal cro = rotate ? halfPi * (hash.mid(3, 1).toInt(&ok, 16) & 3) : 0; // Corner sprite rotation
	qreal sro = rotate ? halfPi * (hash.mid(4, 1).toInt(&ok, 16) & 3) : 0; // Side sprite rotation
	quint8 xbg = hash.mid(5, 1).toInt(&ok, 16) % 2; // Center sprite background

	// Corner sprite foreground color
	quint8 cfr = hash.mid(6, 2).toInt(&ok, 16);
	quint8 cfg = hash.mid(8, 2).toInt(&ok, 16);
	quint8 cfb = hash.mid(10, 2).toInt(&ok, 16);

	// Side sprite foreground color
	quint8 sfr = hash.mid(12, 2).toInt(&ok, 16);
	quint8 sfg = hash.mid(14, 2).toInt(&ok, 16);
	quint8 sfb = hash.mid(16, 2).toInt(&ok, 16);

	// Final angle of rotation
	// not used
	//int angle = hash.mid(18, 2).toInt(&ok, 16);

	/* Size of each sprite */
	quint16 size = width / 3;
	quint16 totalsize = width;

	/// start with blank 3x3 identicon
	QPixmap pixmap = QPixmap(totalsize, totalsize);
    pixmap.fill(QColor::fromRgb(230,230,230));

	// Generate corner sprites
	QList<qreal> corner = getSprite(csh, size);
	QColor fillCorner = QColor( cfr, cfg, cfb );
	drawRotatedPolygon(&pixmap, corner, 0, 0, cro, 0, size, fillCorner);
	drawRotatedPolygon(&pixmap, corner, totalsize, 0, cro, 90, size, fillCorner);
	drawRotatedPolygon(&pixmap, corner, totalsize, totalsize, cro, 180, size, fillCorner);
	drawRotatedPolygon(&pixmap, corner, 0, totalsize, cro, 270, size, fillCorner);

	// Draw sides
	QList<qreal> side = getSprite(ssh, size);
	QColor fillSide = QColor( sfr, sfg, sfb);
	drawRotatedPolygon(&pixmap, side, 0, size, sro, 0, size, fillSide);
	drawRotatedPolygon(&pixmap, side, 2 * size, 0, sro, 90, size, fillSide);
	drawRotatedPolygon(&pixmap, side, 3 * size, 2 * size, sro, 180, size, fillSide);
	drawRotatedPolygon(&pixmap, side, size, 3 * size, sro, 270, size, fillSide);

	// Draw center
	QList<qreal> center = getCenter(xsh, size);
	// Make sure there's enough contrast before we use background color of side sprite
	QColor fillCenter;
	if (xbg > 0 && (abs(cfr - sfr) > 127 || abs(cfg - sfg) > 127 || abs(cfb - sfb) > 127)) {
		fillCenter = QColor( sfr, sfg, sfb);
	} else {
		fillCenter = QColor( cfr, cfg, cfb);
	}
	drawRotatedPolygon(&pixmap, center, size, size, 0, 0, size, fillCenter);

	return pixmap.toImage();
}

//static bool CreateIdIcon(const RsGxsId &id, QIcon &idIcon)
//{
//	QPixmap image(IconSize, IconSize);
//	QPainter painter(&image);

//	painter.fillRect(0, 0, IconSize, IconSize, Qt::black);

//    int len = id.SIZE_IN_BYTES;
//    for(int i = 0; i<len; ++i)
//	{
//        unsigned char hex = id.toByteArray()[i];
//        int x = hex & 0xf ;
//        int y = (hex & 0xf0) >> 4 ;
//		painter.fillRect(x, y, x+1, y+1, Qt::green);
//	}
//	idIcon = QIcon(image);
//	return true;
//}

QString GxsIdDetails::getLoadingText(const RsGxsId &id)
{
	return QString("%1... %2").arg(QApplication::translate("GxsIdDetails", "Loading"), QString::fromStdString(id.toStdString().substr(0, 5)));
}

QString GxsIdDetails::getFailedText(const RsGxsId &id)
{
	return QString("%1 ...[%2]").arg(QApplication::translate("GxsIdDetails", "Not found"), QString::fromStdString(id.toStdString().substr(0, 5)));
}

QString GxsIdDetails::getEmptyIdText()
{
	return QApplication::translate("GxsIdDetails", "No Signature");
}

QString GxsIdDetails::getNameForType(GxsIdDetailsType type, const RsIdentityDetails &details)
{
	switch (type) {
	case GXS_ID_DETAILS_TYPE_EMPTY:
		return getEmptyIdText();

	case GXS_ID_DETAILS_TYPE_LOADING:
		return getLoadingText(details.mId);

	case GXS_ID_DETAILS_TYPE_DONE:
		return getName(details);

	case GXS_ID_DETAILS_TYPE_FAILED:
		return getFailedText(details.mId);
	}

	return "";
}

QIcon GxsIdDetails::getLoadingIcon(const RsGxsId &/*id*/)
{
	return QIcon(IMAGE_LOADING);
}

bool GxsIdDetails::MakeIdDesc(const RsGxsId &id, bool doIcons, QString &str, QList<QIcon> &icons, QString& comment)
{
	RsIdentityDetails details;

	if (!rsIdentity->getIdDetails(id, details))
	{
		// std::cerr << "GxsIdTreeWidget::MakeIdDesc() FAILED TO GET ID " << id;
		//std::cerr << std::endl;

		str = getLoadingText(id);

		if (!doIcons)
		{
			icons.push_back(getLoadingIcon(id));
		}

		return false;
	}

	str = getName(details);

	comment += getComment(details);

	if (doIcons)
		getIcons(details, icons);

//	Cyril: I disabled these three which I believe to have been put for testing purposes.
//
//	icons.push_back(QIcon(IMAGE_ANON));
//	icons.push_back(QIcon(IMAGE_ANON));
//	icons.push_back(QIcon(IMAGE_ANON));

//	std::cerr << "GxsIdTreeWidget::MakeIdDesc() ID Ok. Comment: " << comment.toStdString() ;
//	std::cerr << std::endl;

	return true;
}

QString GxsIdDetails::getName(const RsIdentityDetails &details)
{
	QString name = QString::fromUtf8(details.mNickname.c_str());

	std::list<RsRecognTag>::const_iterator it;
	for (it = details.mRecognTags.begin(); it != details.mRecognTags.end(); ++it)
	{
		name += QString(" (%1 %2)").arg(it->tag_class).arg(it->tag_type);
	}

	return name;
}

QString GxsIdDetails::getComment(const RsIdentityDetails &details)
{
	QString comment;

    comment = QString("%1:%2<br/>%3:%4").arg(QApplication::translate("GxsIdDetails", "Identity&nbsp;name"),
	                                        QString::fromUtf8(details.mNickname.c_str()),
                                            QApplication::translate("GxsIdDetails", "Identity&nbsp;Id"),
	                                        QString::fromStdString(details.mId.toStdString()));

	if (details.mPgpLinked)
	{
        comment += QString("<br/>%1:%2 ").arg(QApplication::translate("GxsIdDetails", "Authentication"), QApplication::translate("GxsIdDetails", "Signed&nbsp;by"));

		if (details.mPgpKnown)
		{
			/* look up real name */
			std::string authorName = rsPeers->getGPGName(details.mPgpId);
            comment += QString("%1&nbsp;[%2]").arg(QString::fromUtf8(authorName.c_str()), QString::fromStdString(details.mPgpId.toStdString()));
		}
		else
			comment += QApplication::translate("GxsIdDetails", "unknown Key");
	}
	else
        comment += QString("<br/>%1:&nbsp;%2").arg(QApplication::translate("GxsIdDetails", "Authentication"), QApplication::translate("GxsIdDetails", "anonymous"));

	return comment;
}

void GxsIdDetails::getIcons(const RsIdentityDetails &details, QList<QIcon> &icons)
{
    QPixmap pix ;

    if(details.mAvatar.mSize == 0 || !pix.loadFromData(details.mAvatar.mData, details.mAvatar.mSize, "PNG"))
        pix.convertFromImage(makeDefaultIcon(details.mId));


	QIcon idIcon(pix);
	//CreateIdIcon(id, idIcon);
	icons.push_back(idIcon);

	// ICON Logic.
	QIcon baseIcon;
	if (details.mPgpLinked)
	{
		if (details.mPgpKnown)
			baseIcon = QIcon(IMAGE_PGPKNOWN);
		else
			baseIcon = QIcon(IMAGE_PGPUNKNOWN);
	}
	else
		baseIcon = QIcon(IMAGE_ANON);

	icons.push_back(baseIcon);
	// Add In RecognTags Icons.
	std::list<RsRecognTag>::const_iterator it;
	for (it = details.mRecognTags.begin(); it != details.mRecognTags.end(); ++it)
	{
		QIcon tagIcon;
		if (findTagIcon(it->tag_class, it->tag_type, tagIcon))
		{
			icons.push_back(tagIcon);
		}
	}
}

bool GxsIdDetails::GenerateCombinedIcon(QIcon &outIcon, const QList<QIcon> &icons)
{
	int count = icons.size();
	QPixmap image(IconSize * count, IconSize);
	QPainter painter(&image);

	painter.fillRect(0, 0, IconSize * count, IconSize, Qt::transparent);
	QList<QIcon>::const_iterator it;
	int i = 0;
	for(it = icons.begin(); it != icons.end(); ++it, ++i)
	{
		it->paint(&painter, IconSize * i, 0, IconSize, IconSize);
	}

	outIcon = QIcon(image);
	return true;
}
