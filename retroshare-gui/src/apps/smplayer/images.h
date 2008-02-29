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

#ifndef _IMAGES_H_
#define _IMAGES_H_

#include <QPixmap>
#include <QString>

/* Warning: don't use this until global->preferences is created! */
class Images
{

public:
	static QPixmap icon(QString name, int size=-1, bool png = true);
	static QPixmap flippedIcon(QString name, int size=-1, bool png = true);

	static QPixmap resize(QPixmap *p, int size=20);
	static QPixmap flip(QPixmap *p);

private:
	//! Return the filename for the icon
	static QString filename(const QString & name, bool png);

	//! Try to load an icon. \a icon_name is the filename of the
	//! icon without path. Return a null pixmap if loads fails.
	static QPixmap loadIcon(const QString & icon_name);
};

#endif

