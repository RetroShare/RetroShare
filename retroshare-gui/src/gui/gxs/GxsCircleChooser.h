/*******************************************************************************
 * retroshare-gui/src/gui/gxs/GxsCircleChooser.h                               *
 *                                                                             *
 * Copyright 2012-2013 by Robert Fernie   <retroshare.project@gmail.com>       *
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

#ifndef _GXS_CIRCLES_CHOOSER_H
#define _GXS_CIRCLES_CHOOSER_H

#include "gui/common/RSComboBox.h"

#include <retroshare/rsgxscircles.h>

class GxsCircleChooser : public RSComboBox
{
	Q_OBJECT

public:
	GxsCircleChooser(QWidget *parent = nullptr);

	void loadCircles(const RsGxsCircleId &defaultId);
	bool getChosenCircle(RsGxsCircleId &id);

private:
	void loadGxsCircles();

	RsGxsCircleId mDefaultCircleId;
};

#endif

