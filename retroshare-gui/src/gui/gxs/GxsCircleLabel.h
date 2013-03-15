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


#ifndef _GXS_CIRCLE_LABEL_H
#define _GXS_CIRCLE_LABEL_H

#include <QTimer>
#include <QLabel>
#include <retroshare/rsgxscircles.h>

class GxsCircleLabel : public QLabel
{
        Q_OBJECT

public:
	GxsCircleLabel(QWidget *parent = NULL);

	void setCircleId(const RsGxsCircleId &id);
	bool getCircleId(RsGxsCircleId &id);

private slots:
	void loadGxsCircle();

private:

	QTimer *mTimer;
	RsGxsCircleId mId;
	int mCount;
};

#endif

