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

#ifndef IQTTOASTER_H
#define IQTTOASTER_H

#include <util/Interface.h>

class QString;
class QPixmap;

/**
 * Shows a toaster when a phone call or a chat is incoming.
 *
 * A toaster is a small window in the lower right of the desktop.
 *
 * 
 */
class IQtToaster : Interface {
public:

	/**
	 * Sets the toaster window message.
	 *
	 * @param message toaster message
	 */
	virtual void setMessage(const QString & message) = 0;

	/**
	 * Sets the toaster window picture.
	 *
	 * @param pixmap toaster picture
	 */
	virtual void setPixmap(const QPixmap & pixmap) = 0;

	/**
	 * Shows the toaster window.
	 */
	virtual void show() = 0;

	/**
	 * Closes the toaster window.
	 */
	virtual void close() = 0;
};

#endif	//IQTTOASTER_H
