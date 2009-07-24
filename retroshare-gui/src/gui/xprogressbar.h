/*
 *	xProgressBar: A custom progress bar for Qt 4.
 *	Author: xEsk (Xesc & Technology 2008)
 *
 *	Changelog:
 *
 *	v1.0:
 *	-----
 *		- First release
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

#ifndef XPROGRESSBAR_H
#define XPROGRESSBAR_H
//
#include <QRect>
#include <QColor>
#include <QPainter>
#include <QLinearGradient>
#include <QLocale>
//
class xProgressBar : public QObject
{
Q_OBJECT
	private:
		// progress vlues
		float progressValue;
		int schemaIndex;
		bool displayText;
		int vSpan;
		int hSpan;
		// painter config
		QRect rect;
		QPainter *painter;
		// text color
		QColor textColor;
		// progress colors
		QColor backgroundBorderColor;
		QColor backgroundColor;
		QColor gradBorderColor;
		QColor gradColor1;
		QColor gradColor2;
		// configure the color
		void setColor();
	public:
		xProgressBar(QRect rect, QPainter *painter, int schemaIndex = 0);
		void paint();
		void setColorSchema(const int value);
		void setValue(const float value);
		void setDisplayText(const bool display);
		void setVerticalSpan(const int value);
		void setHorizontalSpan(const int value);
};
#endif
