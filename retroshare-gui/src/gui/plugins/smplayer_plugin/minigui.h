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

#ifndef _MINI_GUI_H_
#define _MINI_GUI_H_

#include "baseguiplus.h"
#include "guiconfig.h"

#define USE_VOLUME_BAR 1

class TimeSliderAction;
class VolumeSliderAction;
class TimeLabelAction;
class FloatingWidget;
class QToolBar;

class MiniGui : public BaseGuiPlus
{
	Q_OBJECT

public:
	MiniGui( QWidget* parent = 0, Qt::WindowFlags flags = 0 );
	~MiniGui();

#if USE_MINIMUMSIZE
	virtual QSize minimumSizeHint () const;
#endif

protected slots:
	void showFloatingControl(QPoint p);
	void hideFloatingControl();

	// Reimplemented:
#if AUTODISABLE_ACTIONS
	virtual void enableActionsOnPlaying();
	virtual void disableActionsOnStop();
#endif

protected:
	virtual void retranslateStrings();

	void createActions();
	void createControlWidget();
	void createFloatingControl();

	void loadConfig();
	void saveConfig();

	// Reimplemented
	virtual void aboutToEnterFullscreen();
	virtual void aboutToExitFullscreen();
	virtual void aboutToEnterCompactMode();
	virtual void aboutToExitCompactMode();

protected:
	QToolBar * controlwidget;

	FloatingWidget * floating_control;

	TimeSliderAction * timeslider_action;
#if USE_VOLUME_BAR
	VolumeSliderAction * volumeslider_action;
#endif
	TimeLabelAction * time_label_action;
};

#endif
