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

#ifndef _PREFADVANCED_H_
#define _PREFADVANCED_H_

#include "ui_prefadvanced.h"
#include "prefwidget.h"

class Preferences;

class PrefAdvanced : public PrefWidget, public Ui::PrefAdvanced
{
	Q_OBJECT

public:
	PrefAdvanced( QWidget * parent = 0, Qt::WindowFlags f = 0 );
	~PrefAdvanced();

	virtual QString sectionName();
	virtual QPixmap sectionIcon();

    // Pass data to the dialog
    void setData(Preferences * pref);

    // Apply changes
    void getData(Preferences * pref);

	bool clearingBackgroundChanged() { return clearing_background_changed; };
	bool colorkeyChanged() { return colorkey_changed; };
	bool monitorAspectChanged() { return monitor_aspect_changed; };

protected:
	virtual void createHelp();

	// Advanced
	void setMonitorAspect(QString asp);
	QString monitorAspect();

	void setClearBackground(bool b);
	bool clearBackground();

	void setUseMplayerWindow(bool v);
	bool useMplayerWindow();

	void setMplayerAdditionalArguments(QString args);
	QString mplayerAdditionalArguments();

	void setMplayerAdditionalVideoFilters(QString s);
	QString mplayerAdditionalVideoFilters();

	void setMplayerAdditionalAudioFilters(QString s);
	QString mplayerAdditionalAudioFilters();

	void setColorKey(unsigned int c);
	unsigned int colorKey();

	// Log options
	void setLogMplayer(bool b);
	bool logMplayer();

	void setLogSmplayer(bool b);
	bool logSmplayer();

	void setLogFilter(QString filter);
	QString logFilter();

	// MPlayer language
	void setEndOfFileText(QString t);
	QString endOfFileText();

	void setNoVideoText(QString t);
	QString noVideoText();

protected:
	virtual void retranslateStrings();

protected slots:
	void on_changeButton_clicked();

private:
	bool clearing_background_changed;
	bool colorkey_changed;
	bool monitor_aspect_changed;
};

#endif
