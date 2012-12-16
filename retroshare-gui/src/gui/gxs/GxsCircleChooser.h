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


#ifndef _GXS_CIRCLES_CHOOSER_H
#define _GXS_CIRCLES_CHOOSER_H

#include <QComboBox>
#include <retroshare/rsgxscircles.h>

class GxsCircleChooser : public QComboBox
{
        Q_OBJECT

public:
	GxsCircleChooser(QWidget *parent = NULL);

	void loadCircles(uint32_t chooserFlags);
	bool getChosenCircle(RsGxsCircleId &id);

private:
	void loadGxsCircles();
	uint32_t mFlags;
};

#endif

