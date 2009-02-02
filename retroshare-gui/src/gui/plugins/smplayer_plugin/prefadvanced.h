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

#ifndef _PREFADVANCED_H_
#define _PREFADVANCED_H_

#include "ui_prefadvanced.h"
#include "prefwidget.h"
#include "config.h"

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

#if REPAINT_BACKGROUND_OPTION
	bool repaintVideoBackgroundChanged() { return repaint_video_background_changed; };
#endif
	bool monitorAspectChanged() { return monitor_aspect_changed; };
#if USE_COLORKEY
	bool colorkeyChanged() { return colorkey_changed; };
#endif

	bool proxyChanged() { return proxy_changed; };

protected:
	virtual void createHelp();

	// Advanced
	void setMonitorAspect(QString asp);
	QString monitorAspect();

#if REPAINT_BACKGROUND_OPTION
	void setRepaintVideoBackground(bool b);
	bool repaintVideoBackground();
#endif

	void setUseMplayerWindow(bool v);
	bool useMplayerWindow();

	// Windows only: pass to mplayer short filenames (8+3)
	void setUseShortNames(bool b);
	bool useShortNames();

	void setMplayerAdditionalArguments(QString args);
	QString mplayerAdditionalArguments();

	void setMplayerAdditionalVideoFilters(QString s);
	QString mplayerAdditionalVideoFilters();

	void setMplayerAdditionalAudioFilters(QString s);
	QString mplayerAdditionalAudioFilters();

#if USE_COLORKEY
	void setColorKey(unsigned int c);
	unsigned int colorKey();
#endif

	void setPreferIpv4(bool b);
	bool preferIpv4();

	void setUseIdx(bool);
	bool useIdx();

	void setUseCorrectPts(bool b);
	bool useCorrectPts();

	void setActionsToRun(QString actions);
	QString actionsToRun();

	// Log options
	void setLogMplayer(bool b);
	bool logMplayer();

	void setLogSmplayer(bool b);
	bool logSmplayer();

	void setLogFilter(QString filter);
	QString logFilter();

    void setSaveMplayerLog(bool b);
    bool saveMplayerLog();

    void setMplayerLogName(QString filter);
    QString mplayerLogName();

	// Proxy
	void setUseProxy(bool b);
	bool useProxy();

	void setProxyHostname(QString host);
	QString proxyHostname();

	void setProxyPort(int port);
	int proxyPort();

	void setProxyUsername(QString username);
	QString proxyUsername();

	void setProxyPassword(QString password);
	QString proxyPassword();

	void setProxyType(int type);
	int proxyType();

protected:
	virtual void retranslateStrings();

protected slots:
	void on_changeButton_clicked();

private:
#if REPAINT_BACKGROUND_OPTION
	bool repaint_video_background_changed;
#endif
	bool monitor_aspect_changed;
#if USE_COLORKEY
	bool colorkey_changed;
#endif
	bool proxy_changed;
};

#endif
