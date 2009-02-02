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

#include "myactiongroup.h"
#include <QAction>
#include <QList>
#include <QWidget>

MyActionGroupItem::MyActionGroupItem(QObject * parent, MyActionGroup *group,
                                     const char * name, 
                                     int data, bool autoadd)
	: MyAction(parent, name, autoadd)
{
	setData(data);
	setCheckable(true);
	if (group) group->addAction(this);
}

MyActionGroupItem::MyActionGroupItem(QObject * parent, MyActionGroup *group,
                                     const QString & text,
                                     const char * name, 
                                     int data, bool autoadd)
	: MyAction(parent, name, autoadd)
{
	setData(data);
	setText(text);
	setCheckable(true);
	if (group) group->addAction(this);
}


MyActionGroup::MyActionGroup( QObject * parent ) : QActionGroup(parent)
{
	setExclusive(true);
	connect( this, SIGNAL(triggered(QAction *)), 
             this, SLOT(itemTriggered(QAction *)) );
}

void MyActionGroup::setChecked(int ID) {
	//qDebug("MyActionGroup::setChecked: ID: %d", ID);

	QList <QAction *> l = actions();
	for (int n=0; n < l.count(); n++) {
		if ( (!l[n]->isSeparator()) && (l[n]->data().toInt() == ID) ) {
			l[n]->setChecked(true);
			return;
		}
	}
}

int MyActionGroup::checked() {
	QAction * a = checkedAction();
	if (a) 
		return a->data().toInt();
	else
		return -1;
}

void MyActionGroup::uncheckAll() {
	QList <QAction *> l = actions();
	for (int n=0; n < l.count(); n++) {
		l[n]->setChecked(false);
	}
}

void MyActionGroup::setActionsEnabled(bool b) {
	QList <QAction *> l = actions();
	for (int n=0; n < l.count(); n++) {
		l[n]->setEnabled(b);
	}
}

void MyActionGroup::clear(bool remove) {
	while (actions().count() > 0) {
		QAction * a = actions()[0];
		if (a) {
			removeAction(a);
			if (remove) a->deleteLater();
		}
	}
}

void MyActionGroup::itemTriggered(QAction *a) {
	qDebug("MyActionGroup::itemTriggered: '%s'", a->objectName().toUtf8().data());
	int value = a->data().toInt();

	qDebug("MyActionGroup::itemTriggered: ID: %d", value);

	emit activated(value);
}

void MyActionGroup::addTo(QWidget *w) {
	w->addActions( actions() );
}

void MyActionGroup::removeFrom(QWidget *w) {
	for (int n=0; n < actions().count(); n++) {
		w->removeAction( actions()[n] );
	}
}

#include "moc_myactiongroup.cpp"
