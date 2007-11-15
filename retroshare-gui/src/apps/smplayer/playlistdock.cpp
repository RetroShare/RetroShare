/*  smplayer, GUI front-end for mplayer.
    Copyright (C) 2007 Ricardo Villalba <rvm@escomposlinux.org>

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

PlaylistDock::PlaylistDock(QWidget * parent, Qt::WindowFlags flags)
	: QDockWidget(parent, flags)
{
	//setSizePolicy( QSizePolicy::Maximum, QSizePolicy::Expanding );
}

PlaylistDock::~PlaylistDock() {
}

void PlaylistDock::closeEvent( QCloseEvent * /*event*/ ) {
	qDebug("PlaylistDock::closeEvent");
	emit closed();
}

void PlaylistDock::showEvent( QShowEvent * /* event */ ) {
	qDebug("PlaylistDock::showEvent: isFloating: %d", isFloating() );

	if (!isFloating()) {
		qDebug(" docked");
		emit docked();
	}
}

void PlaylistDock::hideEvent( QHideEvent * /* event */ ) {
	qDebug("PlaylistDock::hideEvent: isFloating: %d", isFloating() );

	if (!isFloating()) {
		qDebug(" undocked");
		emit undocked();
	}
}


#include "moc_playlistdock.cpp"

