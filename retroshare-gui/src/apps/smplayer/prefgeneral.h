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

#ifndef _PREFGENERAL_H_
#define _PREFGENERAL_H_

#include "ui_prefgeneral.h"
#include "prefwidget.h"
#include "inforeader.h"
#include "preferences.h"

class PrefGeneral : public PrefWidget, public Ui::PrefGeneral
{
	Q_OBJECT

public:
	PrefGeneral( QWidget * parent = 0, Qt::WindowFlags f = 0 );
	~PrefGeneral();

	// Return the name of the section
	virtual QString sectionName();
	// Return the icon of the section
	virtual QPixmap sectionIcon();

	// Pass data to the dialog
	void setData(Preferences * pref);

	// Apply changes
	void getData(Preferences * pref);

protected:
	virtual void createHelp();

	void setDrivers(InfoList vo_list, InfoList ao_list);

	// Tab General
	void setMplayerPath( QString path );
	QString mplayerPath();

	void setScreenshotDir( QString path );
	QString screenshotDir();

	void setVO( QString vo_driver );
	QString VO();

	void setAO( QString ao_driver );
	QString AO();

	void setRememberSettings(bool b);
	bool rememberSettings();

	void setDontRememberTimePos(bool b);
	bool dontRememberTimePos();

	void setAudioLang(QString lang);
	QString audioLang();

	void setSubtitleLang(QString lang);
	QString subtitleLang();

	void setAudioTrack(int track);
	int audioTrack();

	void setSubtitleTrack(int track);
	int subtitleTrack();

	void setCloseOnFinish(bool b);
	bool closeOnFinish();

	void setPauseWhenHidden(bool b);
	bool pauseWhenHidden();

	// Tab video and audio
	void setEq2(bool b);
	bool eq2();

	void setStartInFullscreen(bool b);
	bool startInFullscreen();

	void setDisableScreensaver(bool b);
	bool disableScreensaver();

	void setAutoq(int n);
	int autoq();

	void setSoftVol(bool b);
	bool softVol();

	void setAc3DTSPassthrough(bool b);
	bool Ac3DTSPassthrough();

	void setInitialVolNorm(bool b);
	bool initialVolNorm();

	void setInitialPostprocessing(bool b);
	bool initialPostprocessing();

	void setDirectRendering(bool b);
	bool directRendering();

	void setDoubleBuffer(bool b);
	bool doubleBuffer();

	void setAmplification(int n);
	int amplification();

	void setInitialVolume(int v);
	int initialVolume();

	void setDontChangeVolume(bool b);
	bool dontChangeVolume();

	// Use -volume option
	void setUseVolume(bool b);
	bool useVolume();

	void setAudioChannels(int ID);
	int audioChannels();

	void setScaleTempoFilter(Preferences::OptionState value);
	Preferences::OptionState scaleTempoFilter();

protected:
	virtual void retranslateStrings();

protected slots:
	void on_searchButton_clicked();
	void on_selectButton_clicked();
};

#endif
