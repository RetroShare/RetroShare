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

#ifndef _PREFPERFORMANCE_H_
#define _PREFPERFORMANCE_H_

#include "ui_prefperformance.h"
#include "prefwidget.h"

class Preferences;

class PrefPerformance : public PrefWidget, public Ui::PrefPerformance
{
	Q_OBJECT

public:
	PrefPerformance( QWidget * parent = 0, Qt::WindowFlags f = 0 );
	~PrefPerformance();

	virtual QString sectionName();
	virtual QPixmap sectionIcon();

    // Pass data to the dialog
    void setData(Preferences * pref);

    // Apply changes
    void getData(Preferences * pref);

protected:
	virtual void createHelp();

	void setCacheEnabled(bool b);
	bool cacheEnabled();

	void setCache(int n);
	int cache();

	void setPriority(int n);
	int priority();

	void setFrameDrop(bool b);
	bool frameDrop();

	void setHardFrameDrop(bool b);
	bool hardFrameDrop();

	void setAutoSyncFactor(int factor);
	int autoSyncFactor();

	void setAutoSyncActivated(bool b);
	bool autoSyncActivated();

	void setFastChapterSeeking(bool b);
	bool fastChapterSeeking();

	void setFastAudioSwitching(bool b);
	bool fastAudioSwitching();

	void setUseIdx(bool);
	bool useIdx();

protected:
	virtual void retranslateStrings();
};

#endif
