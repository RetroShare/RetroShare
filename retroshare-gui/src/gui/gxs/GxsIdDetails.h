/*******************************************************************************
 * retroshare-gui/src/gui/gxs/GxsIdDetails.h                                   *
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

#ifndef _GXS_ID_DETAILS_H
#define _GXS_ID_DETAILS_H

#include <QObject>
#include <QMutex>
#include <QVariant>
#include <QIcon>
#include <QString>
#include <QStyledItemDelegate>

#include <retroshare/rsidentity.h>

class QLabel;

enum GxsIdDetailsType
{
	GXS_ID_DETAILS_TYPE_EMPTY,
	GXS_ID_DETAILS_TYPE_LOADING,
	GXS_ID_DETAILS_TYPE_DONE,
	GXS_ID_DETAILS_TYPE_FAILED,
	GXS_ID_DETAILS_TYPE_BANNED
};

typedef void (*GxsIdDetailsCallbackFunction)(GxsIdDetailsType type, const RsIdentityDetails &details, QObject *object, const QVariant &data);

// This class allows to draw the item in a reputation column using an appropriate size. The max_level_to_display parameter allows to replace
// the icon by an empty icon when needed. This allows to keep the focus on the critical icons only.

class ReputationItemDelegate: public QStyledItemDelegate
{
public:
	ReputationItemDelegate(RsReputationLevel max_level_to_display) :
	    mMaxLevelToDisplay(static_cast<uint32_t>(max_level_to_display)) {}

    virtual void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    virtual QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex & /*index*/) const override;

private:
    uint32_t mMaxLevelToDisplay ;
};


class GxsIdDetails : public QObject
{
	Q_OBJECT

public:
    static const int ICON_TYPE_NONE       = 0x0000 ;
    static const int ICON_TYPE_AVATAR     = 0x0001 ;
    static const int ICON_TYPE_PGP        = 0x0002 ;
    static const int ICON_TYPE_RECOGN     = 0x0004 ;
    static const int ICON_TYPE_REPUTATION = 0x0008 ;
    static const int ICON_TYPE_ALL        = 0x000f ;

	GxsIdDetails();
	virtual ~GxsIdDetails();

    enum AvatarSize {
        SMALL   = 0x00,
        MEDIUM  = 0x01,
        LARGE   = 0x02,
        ORIGINAL= 0x03
    };

	static void initialize();
	static void cleanup();

	/* Information */
	static bool MakeIdDesc(const RsGxsId &id, bool doIcons, QString &desc, QList<QIcon> &icons, QString& comment, uint32_t icon_types=ICON_TYPE_ALL);

	static QString getName(const RsIdentityDetails &details);
	static QString getComment(const RsIdentityDetails &details);

    /*!
     * \brief getIcons
     * 			Returns the list of icons to display along with the ID name. The types of icons to show is a compound of the ICON_TYPE_* flags.
     * 			If reputation is needed and exceeds the minimal reputation, an empty/void icon is showsn . This allow to only show reputation for IDs for which a problem exists.
     */
    static void getIcons(const RsIdentityDetails &details, QList<QIcon> &icons, uint32_t icon_types=ICON_TYPE_ALL, uint32_t minimal_required_reputation=0xff);

	static QString getEmptyIdText();
	static QString getLoadingText(const RsGxsId &id);
	static QString getFailedText(const RsGxsId &id);

	static QString getNameForType(GxsIdDetailsType type, const RsIdentityDetails &details);

	static QIcon getLoadingIcon(const RsGxsId &id);
	static QIcon getReputationIcon(
	        RsReputationLevel icon_index, uint32_t min_reputation );

	static void GenerateCombinedPixmap(QPixmap &pixmap, const QList<QIcon> &icons, int iconSize);

    // These two methods use a cache so as to minimize the memory impact of avatars.

    static const QPixmap makeDefaultIcon(const RsGxsId& id, AvatarSize size = MEDIUM);
	static bool loadPixmapFromData(const unsigned char *data, size_t data_len, QPixmap& pix, AvatarSize size = MEDIUM);
    static void checkCleanImagesCache();
    static void debug_dumpImagesCache();

	/* Processing */
	static void enableProcess(bool enable);
	static bool process(const RsGxsId &id, GxsIdDetailsCallbackFunction callback, QObject *object, const QVariant &data = QVariant());

signals:
	void startTimerFromThread();

protected:
	void connectObject_locked(QObject *object, bool doConnect);

	/* Timer */
	virtual void timerEvent(QTimerEvent *event);

private:
	static QList<qreal> getSprite(quint8 shapeType, quint16 size);
	static QList<qreal> getCenter(quint8 shapeType, quint16 size);
	static void fillPoly (QPainter *painter, QList<qreal> sprite);
	static void drawRotatedPolygon(QPixmap *pixmap,
	                         QList<qreal> sprite,
	                         quint16 x, quint16 y,
	                         qreal shapeangle, qreal angle,
	                         quint16 size, QColor fillColor);
	static QPixmap drawIdentIcon(QString hash, quint16 width, bool rotate);

private slots:
	void objectDestroyed(QObject *object);
	void doStartTimer();

protected:
	class CallbackData
	{
	public:
		CallbackData()
		{
			mAttempt = 0;
			mCallback = 0;
			mObject = NULL;
		}

	public:
		int mAttempt;
		RsGxsId mId;
		GxsIdDetailsCallbackFunction mCallback;
		QObject *mObject;
		QVariant mData;
	};

	static GxsIdDetails *mInstance;

	/* Pending data */
	QMap<QObject*,CallbackData> mPendingData;
	QMap<QObject*,CallbackData>::iterator mPendingDataIterator;

    static uint32_t mImagesAllocated;
    static std::map<RsGxsId,std::pair<time_t,QPixmap>[4] > mDefaultIconCache;
    static time_t mLastIconCacheCleaning;

    int mCheckTimerId;
	int mProcessDisableCount;

	/* Thread safe */
    static QMutex mMutex;
    static QMutex mIconCacheMutex;
};

#endif
