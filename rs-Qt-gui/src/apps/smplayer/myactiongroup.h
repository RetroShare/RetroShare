/*  smplayer, GUI front-end for mplayer.
    Copyright (C) 2007 Ricardo Villalba <rvm@escomposlinux.org>

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

#ifndef _MYACTIONGROUP_H_
#define _MYACTIONGROUP_H_

#include <QActionGroup>
#include <QWidget>
#include "myaction.h"

class MyActionGroup;

//! This class makes easy to create actions for MyActionGroup

class MyActionGroupItem : public MyAction
{
public:
	//! Creates a new item.
	/*! \a group is the group where the action will be added, \a data is
	   the ID of the item. If \autoadd is true the action will be added to
       the parent (if it's a QWidget), so the shortcut could work. */
	MyActionGroupItem( QObject * parent, MyActionGroup *group,
                       const char * name, int data, bool autoadd = true );

	//! Creates a new item.
	/*! \a text is the text that the item will have. */
	MyActionGroupItem( QObject * parent, MyActionGroup *group,
                       const QString & text, const char * name, 
                       int data, bool autoadd = true );
};

class QAction;

//! MyActionGroup makes easier to create exclusive menus based on items
//! with an integer data.


class MyActionGroup : public QActionGroup
{
	Q_OBJECT

public:
	MyActionGroup ( QObject * parent );

	//! Looks for the item which ID is \a ID and checks it
	void setChecked(int ID);

	//! Returns the ID of the item checked or -1 if none
	//! is checked
	int checked();

	//! Remove all items. If \a remove is true the actions are also deleted.
	void clear(bool remove);

	//! Adds all actions to the widget
	void addTo(QWidget *);

	//! Remove all actions from the widget
	void removeFrom(QWidget *);

	//! unchecks all items
	void uncheckAll();

signals:
	//! Emitted when an item has been checked
	void activated(int);

protected slots:
	void itemTriggered(QAction *);
};

#endif
