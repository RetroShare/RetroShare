/*******************************************************************************
 * gui/common/StyleElidedLabel.cpp                                             *
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

#include <QFont>
#include "StyledElidedLabel.h"

/** Constructor */
StyledElidedLabel::StyledElidedLabel(QWidget *parent)
    : ElidedLabel(parent), _lastFactor(-1)
{
}

StyledElidedLabel::StyledElidedLabel(const QString &text, QWidget *parent)
    : ElidedLabel(text, parent), _lastFactor(-1)
{
}

void StyledElidedLabel::setFontSizeFactor(int factor)
{
	int newFactor = factor;
	if (factor > 0) {
		if (_lastFactor > 0) newFactor = 100 + factor - _lastFactor;
		_lastFactor = factor;
		QFont f = font();
		qreal fontSize = newFactor * f.pointSizeF() / 100;
		f.setPointSizeF(fontSize);
		setFont(f);
	}
}
