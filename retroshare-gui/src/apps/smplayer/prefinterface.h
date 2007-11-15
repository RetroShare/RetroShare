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

#ifndef _PREFINTERFACE_H_
#define _PREFINTERFACE_H_

#include "ui_prefinterface.h"
#include "prefwidget.h"

class Preferences;

class PrefInterface : public PrefWidget, public Ui::PrefInterface
{
	Q_OBJECT

public:
	PrefInterface( QWidget * parent = 0, Qt::WindowFlags f = 0 );
	~PrefInterface();

	virtual QString sectionName();
	virtual QPixmap sectionIcon();

    // Pass data to the dialog
    void setData(Preferences * pref);

    // Apply changes
    void getData(Preferences * pref);

	bool languageChanged() { return language_changed; };
	bool iconsetChanged() { return iconset_changed; };
	bool recentsChanged() { return recents_changed; };
	bool styleChanged() { return style_changed; };
	bool serverPortChanged() { return port_changed; };

protected:
	virtual void createHelp();
	void createLanguageCombo();

	void setLanguage(QString lang);
	QString language();

	void setIconSet(QString set);
	QString iconSet();

	void setResizeMethod(int v);
	int resizeMethod();

	void setSaveSize(bool b);
	bool saveSize();

	void setStyle(QString style);
	QString style();

	void setUseSingleInstance(bool b);
	bool useSingleInstance();

	void setServerPort(int port);
	int serverPort();

	void setRecentsMaxItems(int n);
	int recentsMaxItems();

	void setSeeking1(int n);
	int seeking1();

	void setSeeking2(int n);
	int seeking2();

	void setSeeking3(int n);
	int seeking3();

	void setSeeking4(int n);
	int seeking4();

	void setDefaultFont(QString font_desc);
	QString defaultFont();

protected slots:
	void on_changeFontButton_clicked();

protected:
	virtual void retranslateStrings();

private:
	bool language_changed;
	bool iconset_changed;
	bool recents_changed;
	bool style_changed;
	bool port_changed;
};

#endif
