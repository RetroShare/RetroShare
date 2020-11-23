/*******************************************************************************
 * gui/common/StyleElidedLabel.h                                               *
 *                                                                             *
 * Copyright (c) 2014, RetroShare Team <retroshare.project@gmail.com>          *
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
