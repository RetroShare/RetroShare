/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2006, crypton
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

#ifndef LOGOBAR_H
#define LOGOBAR_H

#include <QFrame>

#include <string>

class RetroStyleLabel;

class QString;
class QIcon;

/**
 * logo bar inside the login window.
 *
 * 
 * 
 * 
 * 
 *
 * 
 */
class LogoBar : public QFrame {
	Q_OBJECT
public:

	explicit LogoBar(QWidget * parent);

	~LogoBar();

	void setEnabledLogoButton(bool enable);



Q_SIGNALS:

	void logoButtonClicked();



private Q_SLOTS:

	void logoButtonClickedSlot();



private:

	void init();


	
	RetroStyleLabel * _logoButton;


};

#endif	//LOGOBAR_H
