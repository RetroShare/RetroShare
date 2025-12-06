/*******************************************************************************
 * retroshare-gui/configpage.h                                                 *
 *                                                                             *
 * Copyright (c) 2006-2007, crypton    <retroshare.project@gmail.com>          *
 * Copyright (c) 2006, Matt Edman, Justin Hipple                               *
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

#ifndef _CONFIGPAGE_H
#define _CONFIGPAGE_H

#include <iostream>
#include <QWidget>

class ConfigPage : public QWidget
{
	public:
		/** Default Constructor */
		ConfigPage(QWidget *parent = 0, Qt::WindowFlags flags = Qt::WindowFlags()) : QWidget(parent, flags), loaded(false) {}

		/** Pure virtual method. Subclassed pages load their config settings here. */
		virtual void load() = 0;

		/** Pure virtual method. Subclassed pages save their config settings here
		 * and return true if everything was saved successfully. */

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

private:
		virtual bool save(QString &errmsg) { std::cerr << "(EE) save() shoud not be called!" << errmsg.toStdString() << std::endl; return true;}
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

