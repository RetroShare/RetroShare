/*******************************************************************************
 * gui/common/GroupSelectionBox.h                                              *
 *                                                                             *
 * Copyright (C) 2010, Retroshare Team <retroshare.project@gmail.com>          *
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

#include <QListWidget>
#include <QDialog>
#include <retroshare/rsids.h>
#include <retroshare/rsevents.h>

class GroupSelectionBox: public QListWidget
{
	Q_OBJECT

public:
	GroupSelectionBox(QWidget *parent);
    virtual ~GroupSelectionBox();

    static void selectGroups(const std::list<RsNodeGroupId>& default_groups) ;

    void selectedGroupIds(std::list<RsNodeGroupId> &groupIds) const;
	void selectedGroupNames(QList<QString> &groupNames) const;

    void setSelectedGroupIds(const std::list<RsNodeGroupId> &groupIds);

private slots:
	void fillGroups();

private:
    RsEventsHandlerId_t mEventHandlerId ;
};

class GroupSelectionDialog: public QDialog
{
    Q_OBJECT

public:
    GroupSelectionDialog(QWidget *parent) ;
    virtual ~GroupSelectionDialog() ;

    static std::list<RsNodeGroupId> selectGroups(const std::list<RsNodeGroupId>& default_groups) ;

private:
    GroupSelectionBox *mBox ;
};
