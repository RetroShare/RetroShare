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

#ifndef _BASEGUIPLUS_H_
#define _BASEGUIPLUS_H_

#include "basegui.h"
#include <QSystemTrayIcon>
#include <QPoint>
#include "config.h"

class QMenu;
class PlaylistDock;

class BaseGuiPlus : public BaseGui
{
	Q_OBJECT

public:
	BaseGuiPlus( QWidget* parent = 0, Qt::WindowFlags flags = 0 );
	~BaseGuiPlus();

	virtual bool startHidden();

protected:
	virtual void retranslateStrings();

	void loadConfig();
	void saveConfig();
	void updateShowAllAct();

    virtual void aboutToEnterFullscreen();
    virtual void aboutToExitFullscreen();
	virtual void aboutToEnterCompactMode();
	virtual void aboutToExitCompactMode();

	virtual void closeEvent( QCloseEvent * e );

protected slots:
	// Reimplemented methods
	virtual void closeWindow();
	virtual void setWindowCaption(const QString & title);
	virtual void resizeWindow(int w, int h);
	virtual void updateMediaInfo();
	// New
	virtual void trayIconActivated(QSystemTrayIcon::ActivationReason);
	virtual void toggleShowAll();
	virtual void showAll(bool b);
	virtual void quit();

#if DOCK_PLAYLIST
	virtual void showPlaylist(bool b);
	void playlistClosed();

	void stretchWindow();
	void shrinkWindow();
#endif

protected:
	QSystemTrayIcon * tray;
	QMenu * context_menu;

	MyAction * quitAct;
	MyAction * showTrayAct;
	MyAction * showAllAct;

	// To save state
	QPoint mainwindow_pos;
	bool mainwindow_visible;

	QPoint playlist_pos;
	bool trayicon_playlist_was_visible;

	//QPoint infowindow_pos;
	//bool infowindow_visible;

#if DOCK_PLAYLIST
    PlaylistDock * playlistdock;
	bool fullscreen_playlist_was_visible;
	bool fullscreen_playlist_was_floating;
	bool compact_playlist_was_visible;
	bool ignore_playlist_events;
#endif

};

#endif
