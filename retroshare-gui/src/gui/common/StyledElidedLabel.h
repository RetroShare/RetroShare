/****************************************************************
 * This file is distributed under the following license:
 *
 * Copyright (c) 2014, RetroShare Team
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
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

#ifndef _STYLEDELIDEDLABEL_H
#define _STYLEDELIDEDLABEL_H

#include "ElidedLabel.h"

class StyledElidedLabel : public ElidedLabel
{
	Q_OBJECT
	Q_PROPERTY(int fontSizeFactor READ fontSizeFactor WRITE setFontSizeFactor)

public:
	StyledElidedLabel(QWidget *parent = NULL);
	StyledElidedLabel(const QString &text, QWidget *parent = NULL);

	void setFontSizeFactor(int factor);
	int fontSizeFactor() {return _lastFactor;}

private:
	int _lastFactor;
};

#endif
