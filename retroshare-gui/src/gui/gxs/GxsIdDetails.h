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

#ifndef _GXS_ID_DETAILS_H
#define _GXS_ID_DETAILS_H

#include <QObject>
#include <QMutex>
#include <QVariant>
#include <QIcon>
#include <QString>

#include <retroshare/rsidentity.h>

class QLabel;

enum GxsIdDetailsType
{
	GXS_ID_DETAILS_TYPE_EMPTY,
	GXS_ID_DETAILS_TYPE_LOADING,
	GXS_ID_DETAILS_TYPE_DONE,
	GXS_ID_DETAILS_TYPE_FAILED
};

typedef void (*GxsIdDetailsCallbackFunction)(GxsIdDetailsType type, const RsIdentityDetails &details, QObject *object, const QVariant &data);

class GxsIdDetails : public QObject
{
	Q_OBJECT

public:
	GxsIdDetails();
	virtual ~GxsIdDetails();

	/* Information */
	static bool MakeIdDesc(const RsGxsId &id, bool doIcons, QString &desc, QList<QIcon> &icons, QString& comment);

	static QString getName(const RsIdentityDetails &details);
	static QString getComment(const RsIdentityDetails &details);
	static void getIcons(const RsIdentityDetails &details, QList<QIcon> &icons);

	static QString getEmptyIdText();
	static QString getLoadingText(const RsGxsId &id);
	static QString getFailedText(const RsGxsId &id);

	static QString getNameForType(GxsIdDetailsType type, const RsIdentityDetails &details);

	static QIcon getLoadingIcon(const RsGxsId &id);

	static bool GenerateCombinedIcon(QIcon &outIcon, const QList<QIcon> &icons);

	//static QImage makeDefaultIcon(const RsGxsId& id);
    static QImage makeDefaultIcon(const RsGxsId& id);

	/* Processing */
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
	static QImage drawIdentIcon(QString hash, quint16 width, bool rotate);

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
	QList<CallbackData> mPendingData;
	int mCheckTimerId;

	/* Thread safe */
	QMutex mMutex;
};

#endif
