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

#include "videopreview.h"
#include <QApplication>
#include <QWidget>
#include <QSettings>
#include <stdio.h>

int main( int argc, char ** argv ) 
{
	QApplication a( argc, argv );

	QString filename;

	if (a.arguments().count() > 1) {
		filename = a.arguments()[1];
	}

	QSettings set(QSettings::IniFormat, QSettings::UserScope, "RVM", "videopreview");

	VideoPreview vp("mplayer");
	vp.setSettings(&set);

	if (!filename.isEmpty())
		vp.setVideoFile(filename);

	/*
	vp.setGrid(4,5);
	vp.setMaxWidth(800);
	vp.setDisplayOSD(true);
	*/
	//vp.setAspectRatio( 2.35 );

	if ( (vp.showConfigDialog()) && (vp.createThumbnails()) ) {
		vp.show();
		vp.adjustWindowSize();
		return a.exec();
	}

	return 0;
}
