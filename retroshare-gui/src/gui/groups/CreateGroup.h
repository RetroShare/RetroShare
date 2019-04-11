/*******************************************************************************
 * retroshare-gui/src/gui/Groups/CreateGroup.h                                 *
 *                                                                             *
 * Copyright 2006-2010 by Retroshare Team <retroshare.project@gmail.com>       *
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

#ifndef _CREATEGROUP_H
#define _CREATEGROUP_H

#include "ui_CreateGroup.h"

class CreateGroup : public QDialog
{
	Q_OBJECT

public:
	/** Default constructor */
    CreateGroup(const RsNodeGroupId &groupId, QWidget *parent = 0);
	/** Default destructor */
	~CreateGroup();

private slots:
	void changeGroup();
	void groupNameChanged(QString);

private:
    RsNodeGroupId mGroupId;
	QStringList mUsedGroupNames;
	bool mIsStandard;

	/** Qt Designer generated object */
	Ui::CreateGroup ui;
};

#endif
