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
#include "deviceinfo.h"
#include "preferences.h"

#ifdef Q_OS_WIN
#define USE_DSOUND_DEVICES 1
#else
#define USE_ALSA_DEVICES 1
#define USE_XV_ADAPTORS 1
#endif

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

    bool fileSettingsMethodChanged() { return filesettings_method_changed; };

protected:
	virtual void createHelp();

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

	void setRememberTimePos(bool b);
	bool rememberTimePos();

	void setFileSettingsMethod(QString method);
	QString fileSettingsMethod();

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

	void setBlackbordersOnFullscreen(bool b);
	bool blackbordersOnFullscreen();

	void setAutoq(int n);
	int autoq();

	void setSoftVol(bool b);
	bool softVol();

	void setUseAudioEqualizer(bool b);
	bool useAudioEqualizer();

	void setAc3DTSPassthrough(bool b);
	bool Ac3DTSPassthrough();

	void setInitialVolNorm(bool b);
	bool initialVolNorm();

	void setInitialPostprocessing(bool b);
	bool initialPostprocessing();

	void setInitialDeinterlace(int ID);
	int initialDeinterlace();

	void setInitialZoom(double v);
	double initialZoom();

	void setDirectRendering(bool b);
	bool directRendering();

	void setDoubleBuffer(bool b);
	bool doubleBuffer();

	void setUseSlices(bool b);
	bool useSlices();

	void setAmplification(int n);
	int amplification();

	void setInitialVolume(int v);
	int initialVolume();

	void setDontChangeVolume(bool b);
	bool dontChangeVolume();

	// Use -volume option
	void setUseVolume(Preferences::OptionState value);
	Preferences::OptionState useVolume();

	void setAudioChannels(int ID);
	int audioChannels();

	void setScaleTempoFilter(Preferences::OptionState value);
	Preferences::OptionState scaleTempoFilter();

protected slots:
	void vo_combo_changed(int);
	void ao_combo_changed(int);

protected:
	virtual void retranslateStrings();
	void updateDriverCombos();

	InfoList vo_list;
	InfoList ao_list;
	
#if USE_DSOUND_DEVICES
	DeviceList dsound_devices;
#endif

#if USE_ALSA_DEVICES
	DeviceList alsa_devices;
#endif
#if USE_XV_ADAPTORS
	DeviceList xv_adaptors;
#endif

private:
	bool filesettings_method_changed;
};

#endif
