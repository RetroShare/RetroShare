/***************************************************************************
 *   Copyright (C) 2008 by normal   *
 *   normal@Desktop2   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef SETTINGSGUI_H
#define SETTINGSGUI_H

#include "ui_form_settingsgui.h"
#include <QtGui>
#include <QSettings>


class form_settingsgui : public QDialog, private Ui::form_settingsgui
{
    	Q_OBJECT

public:
	form_settingsgui(QWidget *parent = 0, Qt::WFlags flags = 0);
	~form_settingsgui();

private slots:
	void loadSettings();
	void saveSettings();
	void on_styleCombo_activated(const QString &styleName);
	void on_styleSheetCombo_activated(const QString &styleSheetName);
	
	void on_cmd_openFile();
	void on_cmd_openFile2();
	void on_cmd_openFile3();
	void on_cmd_openFile4();
	void on_cmd_openFile5();
	void on_cmd_openFile6();

private:
	QSettings* settings;
	void loadStyleSheet(const QString &sheetName);
  void loadqss();

};


#endif

