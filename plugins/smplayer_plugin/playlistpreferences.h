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

#ifndef _PLAYLISTPREFERENCES_H_
#define _PLAYLISTPREFERENCES_H_

#include "ui_playlistpreferences.h"

class PlaylistPreferences : public QDialog, public Ui::PlaylistPreferences
{
	Q_OBJECT

public:
	PlaylistPreferences( QWidget* parent = 0, Qt::WindowFlags f = 0 );
	~PlaylistPreferences();

	void setDirectoryRecursion(bool b);
	bool directoryRecursion();

	void setAutoGetInfo(bool b);
	bool autoGetInfo();

	void setSavePlaylistOnExit(bool b);
	bool savePlaylistOnExit();

	void setPlayFilesFromStart(bool b);
	bool playFilesFromStart();

	virtual QSize sizeHint () const;
};

#endif

