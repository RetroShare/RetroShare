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

#ifndef _PREFERENCESDIALOG_H_
#define _PREFERENCESDIALOG_H_

#include "ui_preferencesdialog.h"

#ifdef Q_OS_WIN
#define USE_ASSOCIATIONS 1
#endif

class QTextBrowser;
class QPushButton;

class PrefWidget;
class PrefGeneral;
class PrefDrives;
class PrefPerformance;
class PrefSubtitles;
class PrefInterface;
class PrefInput;
class PrefAdvanced;
class PrefAssociations;

class Preferences;


class PreferencesDialog : public QDialog, public Ui::PreferencesDialog
{
	Q_OBJECT

public:
	enum Section { General=0, Drives=1, Performance=2,
                   Subtitles=3, Gui=4, Mouse=5, Advanced=6, Associations=7 };

	PreferencesDialog( QWidget * parent = 0, Qt::WindowFlags f = 0 );
	~PreferencesDialog();

	PrefInterface * mod_interface() { return page_interface; };
	PrefInput * mod_input() { return page_input; };
	PrefAdvanced * mod_advanced() { return page_advanced; };

	void addSection(PrefWidget *w);

	// Pass data to the standard dialogs
	void setData(Preferences * pref);

	// Apply changes
	void getData(Preferences * pref);

	// Return true if the mplayer process should be restarted.
	bool requiresRestart();

public slots:
	void showSection(Section s);

	virtual void accept(); // Reimplemented to send a signal
	virtual void reject();

signals:
	void applied();

protected:
	virtual void retranslateStrings();
	virtual void changeEvent ( QEvent * event ) ;

protected slots:
	void apply();
	void showHelp();

protected:
	PrefGeneral * page_general;
	PrefDrives * page_drives;
	PrefPerformance * page_performance;
	PrefSubtitles * page_subtitles;
	PrefInterface * page_interface;
	PrefInput * page_input;
	PrefAdvanced * page_advanced;

#if USE_ASSOCIATIONS
	PrefAssociations* page_associations; 
#endif

	QTextBrowser * help_window;

private:
    QPushButton * okButton;
    QPushButton * cancelButton;
	QPushButton * applyButton;
    QPushButton * helpButton;
};

#endif
