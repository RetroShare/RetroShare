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

#ifndef _DEFAULTGUI_H_
#define _DEFAULTGUI_H_

#include "config.h"
#include "baseguiplus.h"
#include <QPoint>

class QLabel;
class QToolBar;
class TimeSlider;
class MySlider;
class QPushButton;
class QResizeEvent;
class FloatingControl;
class MyAction;
class QMenu;

class DefaultGui : public BaseGuiPlus
{
	Q_OBJECT

public:
	DefaultGui( QWidget* parent = 0, Qt::WindowFlags flags = 0 );
	~DefaultGui();

public slots:
	//virtual void showPlaylist(bool b);
	virtual void showMainToolbar(bool b);
	virtual void showLanguageToolbar(bool b);

protected:
	void closeEvent (QCloseEvent * event);
	virtual void retranslateStrings();
	virtual QMenu * createPopupMenu();

	void createStatusBar();
	void createMainToolBars();
	void createControlWidget();
	void createControlWidgetMini();
	void createFloatingControl();
	void createActions();
	void createMenus();

	void loadConfig();
	void saveConfig();

    virtual void aboutToEnterFullscreen();
    virtual void aboutToExitFullscreen();
    virtual void aboutToEnterCompactMode();
    virtual void aboutToExitCompactMode();

	virtual void resizeEvent( QResizeEvent * );
	/* virtual void closeEvent( QCloseEvent * ); */

protected slots:
	virtual void updateWidgets();
	virtual void displayTime(double sec, int perc, QString text);
	virtual void displayFrame(int frame);

	virtual void showFloatingControl(QPoint p);
	virtual void showFloatingMenu(QPoint p);
	virtual void hideFloatingControls();

protected:
	QLabel * time_display;
	QLabel * frame_display;

	QToolBar * controlwidget;
	QToolBar * controlwidget_mini;

	QToolBar * toolbar1;
	QToolBar * toolbar2;

	QPushButton * select_audio;
	QPushButton * select_subtitle;

	TimeSlider * timeslider;
	TimeSlider * timeslider_mini;

	MySlider * volumeslider;
	MySlider * volumeslider_mini;

	FloatingControl * floating_control;

	MyAction * showMainToolbarAct;
	MyAction * showLanguageToolbarAct;

	QMenu * toolbar_menu;

	int last_second;

	// Properties to save
	bool show_main_toolbar;
	bool show_language_toolbar;

	int floating_control_width; // Percentage of screen
};

#endif
