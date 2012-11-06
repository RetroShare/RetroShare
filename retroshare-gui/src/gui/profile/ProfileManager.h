/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2012, RetroShare Team
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


#ifndef _PROFILEMANAGER_H
#define _PROFILEMANAGER_H

#include <retroshare/rstypes.h>

#include "ui_ProfileManager.h"

class ProfileManager : public QDialog
{
	Q_OBJECT

public:
	/** Default constructor */
	ProfileManager(QWidget *parent = 0);

private slots:
	void identityTreeWidgetCostumPopupMenu( QPoint point );
	void identityItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous);

	void selectFriend();
	void importIdentity();
	void exportIdentity();
	void checkChanged(int i);
	void newIdentity();

private:
	QTreeWidgetItem *getCurrentIdentity();

	void fillIdentities();

	/** Qt Designer generated object */
	Ui::ProfileManager ui;

	bool genNewGPGKey;
};

#endif

