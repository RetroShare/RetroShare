/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2006,2007 crypton
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

#ifndef ANIMATEDBUTTON_H
#define ANIMATEDBUTTON_H

#include <util/rsqtutildll.h>

#include <util/NonCopyable.h>

#include <QtCore/QObject>

class QAbstractButton;
class QMovie;
class QString;

/**
 * QAbstractButton with an animated icon (QMovie).
 *
 * Animated icon file should be a .mng file.
 *
 * 
 */
class RSQTUTIL_API AnimatedButton : public QObject, NonCopyable {
	Q_OBJECT
public:

	/**
	 * Constructs a animated button.
	 *
	 * @param button button' icon to animate
	 * @param animatedIconFilename mng filename to animate the button icon
	 */
	AnimatedButton(QAbstractButton * button, const QString & animatedIconFilename);

	~AnimatedButton();

private Q_SLOTS:

	/**
	 * Button icon should be updated.
	 */
	void updateButtonIcon();

private:

	QAbstractButton * _button;

	QMovie * _animatedIcon;
};

#endif	//ANIMATEDBUTTON_H
