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


#ifndef _GXS_ID_CHOOSER_H
#define _GXS_ID_CHOOSER_H

#include <QComboBox>
#include <retroshare/rsidentity.h>

#define IDCHOOSER_ID_REQUIRED	0x0001
#define IDCHOOSER_ANON_DEFAULT  0x0002

class GxsIdChooser : public QComboBox
{
        Q_OBJECT

public:
	GxsIdChooser(QWidget *parent = NULL);

	void loadIds(uint32_t chooserFlags, RsGxsId defId);
	bool getChosenId(RsGxsId &id);

private slots:
	void timer();

private:
	void loadPrivateIds();
	void addPrivateId(const RsGxsId &gxsId, bool replace);
	bool MakeIdDesc(const RsGxsId &id, QString &desc);

	uint32_t mFlags;
	RsGxsId mDefaultId;

	QList<RsGxsId> mPendingId;
	QTimer *mTimer;
	unsigned int mTimerCount;
};

#endif

