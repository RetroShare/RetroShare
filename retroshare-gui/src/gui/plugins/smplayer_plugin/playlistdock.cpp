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

#include "playlistdock.h"
#include <QCloseEvent>

PlaylistDock::PlaylistDock(QWidget * parent, Qt::WindowFlags flags)
	: QDockWidget(parent, flags)
{
	//setSizePolicy( QSizePolicy::Maximum, QSizePolicy::Expanding );
	setAcceptDrops(true); // Fix for Qt 4.4, otherwise the playlist doesn't accept drops...
}

PlaylistDock::~PlaylistDock() {
}

void PlaylistDock::closeEvent( QCloseEvent * e ) {
	qDebug("PlaylistDock::closeEvent");
	emit closed();
	e->accept();
}

#if QT_VERSION < 0x040300
void PlaylistDock::showEvent( QShowEvent * /* event */ ) {
	qDebug("PlaylistDock::showEvent");
	emit visibilityChanged(true);
}

void PlaylistDock::hideEvent( QHideEvent * /* event */ ) {
	qDebug("PlaylistDock::hideEvent");
	emit visibilityChanged(false);
}
#endif


#include "moc_playlistdock.cpp"

