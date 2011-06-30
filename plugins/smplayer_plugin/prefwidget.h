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

#ifndef _PREFWIDGET_H_
#define _PREFWIDGET_H_

#include <QWidget>
#include <QPixmap>
#include <QString>


#define TEST_AND_SET( Pref, Dialog ) \
	if ( Pref != Dialog ) { Pref = Dialog; requires_restart = TRUE; }

class QEvent;

class PrefWidget : public QWidget 
{

public:
	PrefWidget(QWidget * parent = 0, Qt::WindowFlags f = 0 );
	~PrefWidget();

	// Return the name of the section
	virtual QString sectionName();

	virtual QPixmap sectionIcon();

	// Return true if the changes made require to restart the mplayer
	// process. Should be call just after the changes have been applied.
	virtual bool requiresRestart() { return requires_restart; };

	virtual QString help() { return help_message; };

protected:
	virtual void retranslateStrings();
	virtual void changeEvent ( QEvent * event ) ;

	// Help
	void addSectionTitle(const QString & title);
	void setWhatsThis( QWidget *w, const QString & title, const QString & text);
	void clearHelp();
	
	virtual void createHelp();

	bool requires_restart;

private:
	QString help_message;
};

#endif
