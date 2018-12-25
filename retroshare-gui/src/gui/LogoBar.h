/*******************************************************************************
 * gui/LogoBar.h                                                               *
 *                                                                             *
 * Copyright (c) 2006 Crypton          <retroshare.project@gmail.com>          *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Affero General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/

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

	LogoBar(QWidget * parent);

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
