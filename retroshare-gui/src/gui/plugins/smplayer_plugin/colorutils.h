/*  smplayer, GUI front-end for mplayer.
    Copyright (C) 2006-2008 Ricardo Villalba <rvm@escomposlinux.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef _COLORUTILS_H_
#define _COLORUTILS_H_

#include <QString>

#ifndef Q_OS_WIN
#define COLOR_OUTPUT_SUPPORT 1
#endif

class QWidget;
class QColor;

class ColorUtils {

public:

	//! Returns a string suitable to be used for -ass-color
	static QString colorToRRGGBBAA(unsigned int color);
	static QString colorToRRGGBB(unsigned int color);

	//! Returns a string suitable to be used for -colorkey
	static QString colorToRGB(unsigned int color);

	static QString colorToAABBGGRR(unsigned int color);

	//! Changes the foreground color of the specified widget
	static void setForegroundColor(QWidget * w, const QColor & color);

	//! Changes the background color of the specified widget
	static void setBackgroundColor(QWidget * w, const QColor & color);

    /**
     ** \brief Strip colors and tags from MPlayer output lines
     **
     ** Some MPlayer configurations (configured with --enable-color-console)
     ** use colored/tagged console output. This function removes those colors
     ** and tags.
     **
     ** \param s The string to strip colors and tags from
     ** \return Returns a clean string (no colors, no tags)
     */
#if COLOR_OUTPUT_SUPPORT
    static QString stripColorsTags(QString s);
#endif

};

#endif
