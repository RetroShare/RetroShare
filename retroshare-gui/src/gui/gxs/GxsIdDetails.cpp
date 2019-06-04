/*******************************************************************************
 * retroshare-gui/src/gui/gxs/GxsIdDetails.cpp                                 *
 *                                                                             *
 * Copyright 2012-2013 by Robert Fernie     <retroshare.project@gmail.com>     *
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

#include <QApplication>
#include <QThread>
#include <QTimerEvent>
#include <QMutexLocker>

#include <math.h>
#include <util/rsdir.h>
#include "gui/common/AvatarDialog.h"
#include "GxsIdDetails.h"
#include "retroshare-gui/RsAutoUpdatePage.h"

#include <retroshare/rspeers.h>

#include <iostream>
#include <QPainter>

/* Images for tag icons */
#define IMAGE_LOADING     ":/images/folder-draft.png"
#define IMAGE_PGPKNOWN    ":/images/contact.png"
#define IMAGE_PGPUNKNOWN  ":/images/tags/pgp-unknown.png"
#define IMAGE_ANON        ":/images/tags/anon.png"
#define IMAGE_BANNED      ":/icons/biohazard_red.png"

#define IMAGE_DEV_AMBASSADOR     ":/images/tags/dev-ambassador.png"
#define IMAGE_DEV_CONTRIBUTOR    ":/images/tags/vote_down.png"
#define IMAGE_DEV_TRANSLATOR     ":/images/tags/dev-translator.png"
#define IMAGE_DEV_PATCHER        ":/images/tags/dev-patcher.png"
#define IMAGE_DEV_DEVELOPER      ":/images/tags/developer.png"

#define REPUTATION_LOCALLY_POSITIVE_ICON      ":/icons/bullet_green_yellow_star_128.png"
#define REPUTATION_REMOTELY_POSITIVE_ICON     ":/icons/bullet_green_128.png"
#define REPUTATION_NEUTRAL_ICON               ":/icons/bullet_grey_128.png"
#define REPUTATION_REMOTELY_NEGATIVE_ICON     ":/icons/biohazard_yellow.png"
#define REPUTATION_LOCALLY_NEGATIVE_ICON      ":/icons/biohazard_red.png"
#define REPUTATION_VOID                       ":/icons/void_128.png"

#define TIMER_INTERVAL              500
#define MAX_ATTEMPTS                10
#define MAX_PROCESS_COUNT_PER_TIMER 50

//const int kRecognTagClass_DEVELOPMENT = 1;
//
//const int kRecognTagType_Dev_Ambassador 	= 1;
//const int kRecognTagType_Dev_Contributor 	= 2;
//const int kRecognTagType_Dev_Translator 	= 3;
//const int kRecognTagType_Dev_Patcher     	= 4;
//const int kRecognTagType_Dev_Developer	 	= 5;

uint32_t GxsIdDetails::mImagesAllocated = 0;
time_t GxsIdDetails::mLastIconCacheCleaning = time(NULL);
std::map<RsGxsId,std::pair<time_t,QPixmap>[4] > GxsIdDetails::mDefaultIconCache ;

#define ICON_CACHE_STORAGE_TIME 		  60
#define DELAY_BETWEEN_ICON_CACHE_CLEANING 30

void ReputationItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	Q_ASSERT(index.isValid());

	QStyleOptionViewItemV4 opt = option;
	initStyleOption(&opt, index);
	// disable default icon
	opt.icon = QIcon();
	// draw default item
	QApplication::style()->drawControl(QStyle::CE_ItemViewItem, &opt, painter, 0);

	const QRect r = option.rect;

	// get pixmap
	unsigned int icon_index = qvariant_cast<unsigned int>(index.data(Qt::DecorationRole));

	if(icon_index > mMaxLevelToDisplay)
		return ;

	QIcon icon = GxsIdDetails::getReputationIcon(
	            RsReputationLevel(icon_index), 0xff );

	QPixmap pix = icon.pixmap(r.size());

	// draw pixmap at center of item
	const QPoint p = QPoint((r.width() - pix.width())/2, (r.height() - pix.height())/2);
	painter->drawPixmap(r.topLeft() + p, pix);
}

/* The global object */
GxsIdDetails *GxsIdDetails::mInstance = NULL ;

GxsIdDetails::GxsIdDetails()
    : QObject()
{
	mCheckTimerId = 0;
	mProcessDisableCount = 0;
    mPendingDataIterator = mPendingData.end() ;

	connect(this, SIGNAL(startTimerFromThread()), this, SLOT(doStartTimer()));
}

GxsIdDetails::~GxsIdDetails()
{
}

void GxsIdDetails::initialize()
{
	if (mInstance) {
		return;
	}

	mInstance = new GxsIdDetails;
}

void GxsIdDetails::cleanup()
{
	if (!mInstance) {
		return;
	}

	delete(mInstance);
	mInstance = NULL;
}

void GxsIdDetails::objectDestroyed(QObject *object)
{
	if (!object) {
		return;
	}

	QMutexLocker lock(&mMutex);

	/* Object is about to be destroyed, remove it from pending list */
	QList<CallbackData>::iterator dataIt;
    
    	QMap<QObject*,CallbackData>::iterator it = mPendingData.find(object) ;
        
        if(it != mPendingData.end())
        {
            if(it == mPendingDataIterator)
                mPendingDataIterator = mPendingData.erase(it) ;
            else
                mPendingData.erase(it) ;
        }
}

void GxsIdDetails::connectObject_locked(QObject *object, bool doConnect)
{
	if (!object) {
		return;
	}

	/* Search Object in pending list */
    
	if(mPendingData.find(object) == mPendingData.end())	// force disconnect when not in the list
	   doConnect = false ;

	if (doConnect) {
		connect(object, SIGNAL(destroyed(QObject*)), this, SLOT(objectDestroyed(QObject*)));
	} else {
		disconnect(object, SIGNAL(destroyed(QObject*)), this, SLOT(objectDestroyed(QObject*)));
	}
}

void GxsIdDetails::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == mCheckTimerId) {
		if (!RsAutoUpdatePage::eventsLocked()) {
			/* Stop timer */
			killTimer(mCheckTimerId);
			mCheckTimerId = 0;

			if (rsIdentity) {
				QMutexLocker lock(&mMutex);

				if (mProcessDisableCount == 0) {
					if (!mPendingData.empty()) {
						/* Check pending id's */
						int processed = qMin(MAX_PROCESS_COUNT_PER_TIMER, mPendingData.size());
                        
						while (!mPendingData.isEmpty()) {
							if (processed-- <= 0) {
								break;
							}
                            
                            				if(mPendingDataIterator == mPendingData.end())
                                                		mPendingDataIterator = mPendingData.begin() ;

							CallbackData &pendingData = *mPendingDataIterator;

							RsIdentityDetails details;
							if (rsIdentity->getIdDetails(pendingData.mId, details)) {
								/* Got details */
								pendingData.mCallback(GXS_ID_DETAILS_TYPE_DONE, details, pendingData.mObject, pendingData.mData);

								QObject *object = pendingData.mObject;
								connectObject_locked(object, false);

								mPendingDataIterator = mPendingData.erase(mPendingDataIterator);

								continue;
							}

							if (++pendingData.mAttempt > MAX_ATTEMPTS) {
								/* Max attempts reached, stop trying */
								details.mId = pendingData.mId;
								pendingData.mCallback(GXS_ID_DETAILS_TYPE_FAILED, details, pendingData.mObject, pendingData.mData);

								QObject *object = pendingData.mObject;
								connectObject_locked(object, false);

								mPendingDataIterator = mPendingData.erase(mPendingDataIterator);

								continue;
							}
                            
                            				++mPendingDataIterator ;
                            
							//mPendingData.move(0, mPendingData.size() - 1);
						}
					}
				}
			}

			QMutexLocker lock(&mMutex);

			if (mPendingData.empty()) {
				/* All done */
			} else {
				/* Start timer */
				doStartTimer();
			}
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

void GxsIdDetails::enableProcess(bool enable)
{
	if (!mInstance) {
		return;
	}

	QMutexLocker lock(&mInstance->mMutex);

	if (enable) {
		--mInstance->mProcessDisableCount;
		if (mInstance->mProcessDisableCount < 0) {
			mInstance->mProcessDisableCount = 0;
		}
	} else {
		++mInstance->mProcessDisableCount;
	}
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

    	// remove any existing call for this object. This is needed for when the same widget is used to display IDs that vary in time.
	{
		QMutexLocker lock(&mInstance->mMutex);

        	// check if a pending request is not already on its way. If so, replace it.
        
        	QMap<QObject*,CallbackData>::iterator it = mInstance->mPendingData.find(object) ;
            
            	if(it != mInstance->mPendingData.end())
                {
			mInstance->connectObject_locked(object, false);
            
            		if(mInstance->mPendingDataIterator == it)
				mInstance->mPendingDataIterator = mInstance->mPendingData.erase(it) ;
                        else
				mInstance->mPendingData.erase(it) ;
                }
               
		/* Connect signal "destroy" */
	}
	/* Try to get the information */
	// the idea behind this was, to call the callback directly when the identity is already loaded in librs
	// without one timer tick, but it causes the use of Pixmap in avatars within a threat that is different than
	// the GUI thread, which is not allowed by Qt => some avatars fail to load.

	bool isGuiThread = (QThread::currentThread() == qApp->thread());
	if (isGuiThread && !RsAutoUpdatePage::eventsLocked() && rsIdentity && rsIdentity->getIdDetails(id, details)) {
		callback(GXS_ID_DETAILS_TYPE_DONE, details, object, data);
		return true;
	}

	details.mId = id;

	/* Add id to the pending list */
	if (!mInstance) {
		/* GxsIdDetails not initialized. Please use ::initialize */
		callback(GXS_ID_DETAILS_TYPE_FAILED, details, object, data);
		return false;
	}

	callback(GXS_ID_DETAILS_TYPE_LOADING, details, object, data);

	CallbackData pendingData;
	pendingData.mId = id;
	pendingData.mCallback = callback;
	pendingData.mObject = object;
	pendingData.mData = data;

	{
		QMutexLocker lock(&mInstance->mMutex);

        	// check if a pending request is not already on its way. If so, replace it.
        
		mInstance->mPendingData[object] = pendingData;
               
		/* Connect signal "destroy" */
		mInstance->connectObject_locked(object, true);
	}

	/* Start timer */
	if (isGuiThread) {
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
const QPixmap GxsIdDetails::makeDefaultIcon(const RsGxsId& id, AvatarSize size)
{
    checkCleanImagesCache();

    // We use a cache for images. QImage has its own smart pointer system, but it does not prevent
    // the same image to be allocated many times. We do this using a cache. The cache is also cleaned-up
    // on a regular time basis so as to get rid of unused images.

    time_t now = time(NULL);

    // now look for the icon

    auto& it = mDefaultIconCache[id];

    if(it[(int)size].second.width() > 0)
    {
        it[(int)size].first = now;
        return it[(int)size].second;
    }

    int S =0;

    switch(size)
    {
    	case SMALL:  S = 16*3 ; break;
    default:
    	case MEDIUM: S = 32*3 ; break;
    	case ORIGINAL:
    	case LARGE:  S = 64*3 ; break;
    }

    QPixmap image = drawIdentIcon(QString::fromStdString(id.toStdString()),S,true);

    it[(int)size] = std::make_pair(now,image);

    return image;
}

void GxsIdDetails::checkCleanImagesCache()
{
    time_t now = time(NULL);

    // cleanup the cache every 10 mins

    if(mLastIconCacheCleaning + DELAY_BETWEEN_ICON_CACHE_CLEANING < now)
    {
        std::cerr << "(II) Cleaning the icons cache." << std::endl;
        int nb_deleted = 0;
        uint32_t size_deleted = 0;
        uint32_t total_size = 0;

        for(auto it(mDefaultIconCache.begin());it!=mDefaultIconCache.end();)
        {
            bool all_empty = true ;

            for(int i=0;i<4;++i)
				if(it->second[i].first + ICON_CACHE_STORAGE_TIME < now && it->second[i].second.isDetached())
				{
                    int s = it->second[i].second.width()*it->second[i].second.height()*4;

					std::cerr << "Deleting pixmap " << it->first << " size " << i << " " << s << " bytes." << std::endl;

                    it->second[i].second = QPixmap();
					it = mDefaultIconCache.erase(it);
					++nb_deleted;
                    size_deleted += s;
				}
				else
                {
					all_empty = false;
                    total_size += it->second[i].second.width()*it->second[i].second.height()*4;
                }

            if(all_empty)
				it = mDefaultIconCache.erase(it);
			else
				++it;
        }

        mLastIconCacheCleaning = now;
        std::cerr << "(II) Removed " << nb_deleted << " (" << size_deleted << " bytes) unused icons. Cache contains " << mDefaultIconCache.size() << " icons (" << total_size << " bytes)"<< std::endl;
    }
}


bool GxsIdDetails::loadPixmapFromData(const unsigned char *data,size_t data_len,QPixmap& pixmap, AvatarSize size)
{
    // The trick below converts the data into an Id that can be read in the image cache. Because this method is mainly dedicated to loading
    // avatars, we could also use the GxsId as id, but the avatar may change in time, so we actually need to make the id from the data itself.

    assert(Sha1CheckSum::SIZE_IN_BYTES >= RsGxsId::SIZE_IN_BYTES);

    Sha1CheckSum chksum = RsDirUtil::sha1sum(data,data_len);
    RsGxsId id(chksum.toByteArray());

    // We use a cache for images. QImage has its own smart pointer system, but it does not prevent
    // the same image to be allocated many times. We do this using a cache. The cache is also cleaned-up
    // on a regular time basis so as to get rid of unused images.

    checkCleanImagesCache();

    // now look for the icon

    time_t now = time(NULL);
    auto& it = mDefaultIconCache[id];

    if(it[(int)size].second.width() > 0)
    {
        it[(int)size].first = now;
		pixmap = it[(int)size].second;

        return true;
    }

    if(! pixmap.loadFromData(data,data_len))
        return false;

    // This resize is here just to prevent someone to explicitely add a huge blank image to screw up the UI

    int wanted_S=0;

    switch(size)
    {
    case ORIGINAL:  wanted_S = 0   ;break;
    case SMALL:     wanted_S = 32  ;break;
    default:
    case MEDIUM:    wanted_S = 64  ;break;
    case LARGE:     wanted_S = 128 ;break;
    }

    if(wanted_S > 0)
		pixmap = pixmap.scaled(wanted_S,wanted_S,Qt::IgnoreAspectRatio,Qt::SmoothTransformation);

    mDefaultIconCache[id][(int)size] = std::make_pair(now,pixmap);

    std::cerr << "Allocated new icon " << id << " size " << (int)size << std::endl;
    return true;
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
QPixmap GxsIdDetails::drawIdentIcon( QString hash, quint16 width, bool rotate)
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

	return pixmap;
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
	return QApplication::translate("GxsIdDetails", "[None]");
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

	case GXS_ID_DETAILS_TYPE_BANNED:
		return tr("[Banned]") ;

	case GXS_ID_DETAILS_TYPE_FAILED:
		return getFailedText(details.mId);
	}

	return "";
}

QIcon GxsIdDetails::getLoadingIcon(const RsGxsId &/*id*/)
{
	return QIcon(IMAGE_LOADING);
}

bool GxsIdDetails::MakeIdDesc(const RsGxsId &id, bool doIcons, QString &str, QList<QIcon> &icons, QString& comment,uint32_t icon_types)
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
		getIcons(details, icons,icon_types);

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
	if( details.mReputation.mOverallReputationLevel ==
	         RsReputationLevel::LOCALLY_NEGATIVE )
		return tr("[Banned]");
    
    	QString name = QString::fromUtf8(details.mNickname.c_str()).left(RSID_MAXIMUM_NICKNAME_SIZE);

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
QString nickname ;

    bool banned = ( details.mReputation.mOverallReputationLevel ==
	                RsReputationLevel::LOCALLY_NEGATIVE );
        
    	if(details.mNickname.empty())
            nickname = tr("[Unknown]") ;
        else if(banned)
            nickname = tr("[Banned]") ;
        else
            nickname = QString::fromUtf8(details.mNickname.c_str()).left(RSID_MAXIMUM_NICKNAME_SIZE) ;

                            
    comment = QString("%1:%2<br/>%3:%4").arg(QApplication::translate("GxsIdDetails", "Identity&nbsp;name"),
                                             nickname,
                                            QApplication::translate("GxsIdDetails", "Identity&nbsp;Id"),
	                                        QString::fromStdString(details.mId.toStdString()));

	if (details.mFlags & RS_IDENTITY_FLAGS_PGP_LINKED)
	{
        comment += QString("<br/>%1: ").arg(QApplication::translate("GxsIdDetails", "Node"));

		if (details.mFlags & RS_IDENTITY_FLAGS_PGP_KNOWN)
		{
			/* look up real name */
			std::string authorName = rsPeers->getGPGName(details.mPgpId);
                        
			comment += QString("%1&nbsp;[%2]").arg(QString::fromUtf8(authorName.c_str()), QString::fromStdString(details.mPgpId.toStdString()));
		}
		else
			comment += QApplication::translate("GxsIdDetails", "unknown Key");
	}
	//else
     //   comment += QString("<br/>%1:&nbsp;%2").arg(QApplication::translate("GxsIdDetails", "Node:"), QApplication::translate("GxsIdDetails", "anonymous"));
	
	if(details.mReputation.mFriendsPositiveVotes || details.mReputation.mFriendsNegativeVotes)
	{
		comment += "<br/>Votes:";
		if(details.mReputation.mFriendsPositiveVotes > 0) comment += " <b>+" + QString::number(details.mReputation.mFriendsPositiveVotes) + "</b>";
		if(details.mReputation.mFriendsNegativeVotes > 0) comment += " <b>-" + QString::number(details.mReputation.mFriendsNegativeVotes) + "</b>";
    }
	return comment;
}

QIcon GxsIdDetails::getReputationIcon(
        RsReputationLevel icon_index, uint32_t min_reputation )
{
	if( static_cast<uint32_t>(icon_index) >= min_reputation )
		return QIcon(REPUTATION_VOID);

	switch(icon_index)
	{
	case RsReputationLevel::LOCALLY_NEGATIVE:
		return QIcon(REPUTATION_LOCALLY_NEGATIVE_ICON);
	case RsReputationLevel::LOCALLY_POSITIVE:
		return QIcon(REPUTATION_LOCALLY_POSITIVE_ICON);
	case RsReputationLevel::REMOTELY_POSITIVE:
		return QIcon(REPUTATION_REMOTELY_POSITIVE_ICON);
	case RsReputationLevel::REMOTELY_NEGATIVE:
		return QIcon(REPUTATION_REMOTELY_NEGATIVE_ICON);
	case RsReputationLevel::NEUTRAL:
		return QIcon(REPUTATION_NEUTRAL_ICON);
	default:
		std::cerr << "Asked for unidentified icon index "
		          << static_cast<uint32_t>(icon_index) << std::endl;
		return QIcon(); // dont draw anything
	}
}

void GxsIdDetails::getIcons(const RsIdentityDetails &details, QList<QIcon> &icons,uint32_t icon_types,uint32_t minimal_required_reputation)
{
    QPixmap pix ;

	if( details.mReputation.mOverallReputationLevel ==
	         RsReputationLevel::LOCALLY_NEGATIVE )
    {
        icons.clear() ;
        icons.push_back(QIcon(IMAGE_BANNED)) ;
        return ;
    }

	if(icon_types & ICON_TYPE_REPUTATION)
        icons.push_back(getReputationIcon(details.mReputation.mOverallReputationLevel,minimal_required_reputation)) ;

    if(icon_types & ICON_TYPE_AVATAR)
    {
        if(details.mAvatar.mSize == 0 || !GxsIdDetails::loadPixmapFromData(details.mAvatar.mData, details.mAvatar.mSize, pix))
#if QT_VERSION < 0x040700
            pix = makeDefaultIcon(details.mId);
#else
            pix = makeDefaultIcon(details.mId);
#endif


        QIcon idIcon(pix);
        //CreateIdIcon(id, idIcon);
        icons.push_back(idIcon);
    }

    if(icon_types & ICON_TYPE_PGP)
    {
        // ICON Logic.
        QIcon baseIcon;
        if (details.mFlags & RS_IDENTITY_FLAGS_PGP_LINKED)
        {
		if (details.mFlags & RS_IDENTITY_FLAGS_PGP_KNOWN)
                baseIcon = QIcon(IMAGE_PGPKNOWN);
            else
                baseIcon = QIcon(IMAGE_PGPUNKNOWN);
        }
        else
            baseIcon = QIcon(IMAGE_ANON);

        icons.push_back(baseIcon);
    }

    if(icon_types & ICON_TYPE_RECOGN)
    {
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
}

void GxsIdDetails::GenerateCombinedPixmap(QPixmap &pixmap, const QList<QIcon> &icons, int iconSize)
{
	int count = icons.size();
	if (count == 0) {
		pixmap = QPixmap();
		return;
	}

	pixmap = QPixmap(iconSize * count, iconSize);
	pixmap.fill(Qt::transparent);
	QPainter painter(&pixmap);

	QList<QIcon>::const_iterator it;
	int i = 0;
	for(it = icons.begin(); it != icons.end(); ++it, ++i)
	{
		it->paint(&painter, iconSize * i, 0, iconSize, iconSize);
	}
}
