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

#ifndef _CONFIG_H_
#define _CONFIG_H_


// SUBTITLES_BY_INDEX 1
// New code for subtitle handling.

#define SUBTITLES_BY_INDEX 1


// CONTROLWIDGET_OVER_VIDEO
// if set to 1, the controlwidget will be shown in fullscreen
// *over* the video (not moving the video) when the user move the mouse 
// to the bottom area of the screen.

#define CONTROLWIDGET_OVER_VIDEO 1


// NEW_CONTROLWIDGET
// New design for the floating control, with only one row.
 
#define NEW_CONTROLWIDGET 1


// DOCK_PLAYLIST
// if 1, the playlist will be docked in the main window, instead
// of being a top level window

#define DOCK_PLAYLIST 1


// STYLE_SWITCHING
// if 1, the preferences dialog will have an option to switch
// the Qt style

#define STYLE_SWITCHING 1


// New code to resize the main window

#define NEW_RESIZE_CODE 1


// Allow to use multiple shortcuts for actions

#define USE_MULTIPLE_SHORTCUTS 1


// EXTERNAL_SLEEP
// if 1, it will be used the function usleep() from unistd.h
// instead of QThread::msleep()
// It can be useful if your Qt doesn't have QThread support.
// Note: not much test it
// Note 2: not used anymore

#define EXTERNAL_SLEEP 0


// USE_SHORTCUTGETTER
// if 1, a new dialog will be used to ask the user for a
// keyshortcut.

#define USE_SHORTCUTGETTER 1


// USE_SUBFONT
// if 1, use -subfont option.

#define USE_SUBFONT 0


// Testing with a QGLWidget (for Windows)
#define USE_GL_WIDGET 0


#endif
