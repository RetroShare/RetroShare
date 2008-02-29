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

#ifndef _PREFINPUT_H_
#define _PREFINPUT_H_

#include "ui_prefinput.h"
#include "prefwidget.h"
#include <QStringList>

class Preferences;

class PrefInput : public PrefWidget, public Ui::PrefInput
{
	Q_OBJECT

public:
	PrefInput( QWidget * parent = 0, Qt::WindowFlags f = 0 );
	~PrefInput();

	virtual QString sectionName();
	virtual QPixmap sectionIcon();

    // Pass data to the dialog
    void setData(Preferences * pref);

    // Apply changes
    void getData(Preferences * pref);

	// Pass action's list to dialog
	/* void setActionsList(QStringList l); */

protected:
	virtual void createHelp();

	void createMouseCombos();

	void setLeftClickFunction(QString f);
	QString leftClickFunction();

	void setDoubleClickFunction(QString f);
	QString doubleClickFunction();

	void setMiddleClickFunction(QString f);
	QString middleClickFunction();

	void setWheelFunction(int function);
	int wheelFunction();

protected:
	virtual void retranslateStrings();
};

#endif
