/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2008 Robert Fernie
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public License
 *  as published by the Free Software Foundation; either version 2.1
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 *  Boston, MA  02110-1301, USA.
 ****************************************************************/


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

