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

#ifndef _PREFSUBTITLES_H_
#define _PREFSUBTITLES_H_

#include "ui_prefsubtitles.h"
#include "prefwidget.h"

class Preferences;
class Encodings;

class PrefSubtitles : public PrefWidget, public Ui::PrefSubtitles
{
	Q_OBJECT

public:
	PrefSubtitles( QWidget * parent = 0, Qt::WindowFlags f = 0 );
	~PrefSubtitles();

	virtual QString sectionName();
	virtual QPixmap sectionIcon();

    // Pass data to the dialog
    void setData(Preferences * pref);

    // Apply changes
    void getData(Preferences * pref);

protected:
	virtual void createHelp();

	void setFontName(QString font_name);
	QString fontName();

	void setFontFile(QString font_file);
	QString fontFile();

	void setUseFontconfig(bool b);
	bool useFontconfig();

	void setFontAutoscale(int n);
	int fontAutoscale();

	void setFontTextscale(double n);
	double fontTextscale();

	void setAssFontScale(double n);
	double assFontScale();

	void setAutoloadSub(bool v);
	bool autoloadSub();

	void setFontEncoding(QString s);
	QString fontEncoding();

	void setUseEnca(bool v);
	bool useEnca();

	void setEncaLang(QString s);
	QString encaLang();

	void setSubPos(int pos);
	int subPos();

	void setUseFontASS(bool v);
	bool useFontASS();

	void setAssLineSpacing(int spacing);
	int assLineSpacing();

	void setFontFuzziness(int n);
	int fontFuzziness();

	void setSubtitlesOnScreenshots(bool b);
	bool subtitlesOnScreenshots();

	void setFreetypeSupport(bool b);
	bool freetypeSupport();

protected slots:
	void on_ass_subs_button_toggled(bool b);
	void on_freetype_check_toggled(bool b);
	void checkBorderStyleCombo( int index );

protected:
	virtual void retranslateStrings();

private:
	Encodings * encodings;
};

#endif
