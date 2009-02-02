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

#include "myaction.h"
#include <QWidget>

MyAction::MyAction ( QObject * parent, const char * name, bool autoadd ) 
	: QAction(parent)
{
	//qDebug("MyAction::MyAction: name: '%s'", name);
	setObjectName(name);
	if (autoadd) addActionToParent();
}


MyAction::MyAction( QObject * parent, bool autoadd )
	: QAction(parent)
{
	//qDebug("MyAction::MyAction: QObject, bool");
	if (autoadd) addActionToParent();
}

MyAction::MyAction(const QString & text, QKeySequence accel, 
                   QObject * parent, const char * name, bool autoadd )
	: QAction(parent)
{
	setObjectName(name);
	setText(text);
	setShortcut(accel);
	if (autoadd) addActionToParent();
}

MyAction::MyAction(QKeySequence accel, QObject * parent, const char * name, 
                   bool autoadd )
	: QAction(parent)
{
	setObjectName(name);
	setShortcut(accel);
	if (autoadd) addActionToParent();
}

MyAction::~MyAction() {
}

void MyAction::addActionToParent() {
	if (parent()) {
		if (parent()->inherits("QWidget")) {
			QWidget *w = static_cast<QWidget*> (parent());
			w->addAction(this);
		}
	}
}

void MyAction::change(const QIcon & icon, const QString & text) {
	setIcon( icon );
	change(text);
}

void MyAction::change(const QString & text ) {
	setText( text );

	QString accel_text = shortcut().toString();

	QString s = text;
	s.replace("&","");
	if (!accel_text.isEmpty()) {
		setToolTip( s + " ("+ accel_text +")");
		setIconText( s );
	}

	/*
	if (text.isEmpty()) {
		QString s = menuText;
		s = s.replace("&","");
		setText( s );

		if (!accel_text.isEmpty())
			setToolTip( s + " ("+ accel_text +")");
	} else {
		setText( text );
		if (!accel_text.isEmpty())
			setToolTip( text + " ("+ accel_text +")");
	}
	*/
}

