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

#ifndef _CONFIGPAGE_H
#define _CONFIGPAGE_H

#include <QWidget>

class ConfigPage : public QWidget
{
	public:
		/** Default Constructor */
		ConfigPage(QWidget *parent = 0, Qt::WindowFlags flags = 0) : QWidget(parent, flags), loaded(false) {}

		/** Pure virtual method. Subclassed pages load their config settings here. */
		virtual void load() = 0;

		/** Pure virtual method. Subclassed pages save their config settings here
		 * and return true if everything was saved successfully. */

		virtual bool save(QString &errmsg) = 0;

		bool wasLoaded() { return loaded ; }

		// Icon to be used to display the config page.
		//
		virtual QPixmap iconPixmap() const = 0 ;

		// Name of the page, to put in the leftside list
		//
		virtual QString pageName() const = 0 ;

		// Text to be used to display in the help browser
		//
		virtual QString helpText() const = 0;

	protected:
		virtual void showEvent(QShowEvent * /*event*/)
		{
			if(!loaded)
			{
				load() ;
				loaded = true ;
			}
		}

		bool loaded;
};

#endif

