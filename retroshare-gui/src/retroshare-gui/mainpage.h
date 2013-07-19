/****************************************************************
 * This file is distributed under the following license:
 *
 * Copyright (c) 2006-2007, crypton
 * Copyright (c) 2006, Matt Edman, Justin Hipple
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 *  Boston, MA  02110-1301, USA.
 ****************************************************************/

#ifndef _MAINPAGE_H
#define _MAINPAGE_H

#include <QWidget>
#include <QTextBrowser>

class UserNotify;
class QPushButton ;

class MainPage : public QWidget
{
	Q_OBJECT 

	public:
		/** Default Constructor */
		MainPage(QWidget *parent = 0, Qt::WindowFlags flags = 0) ;

		virtual void retranslateUi() {}
		virtual UserNotify *getUserNotify(QObject */*parent*/) { return NULL; }

		// Overload this to add some help info  to the page. The way the info is 
		// shown is handled by showHelp() below;
		//
		void registerHelpButton(QPushButton *button, const QString& help_html_text) ;

	private slots:
		void showHelp(bool b) ;

	private:
		QTextBrowser *help_browser ;
};

#endif

