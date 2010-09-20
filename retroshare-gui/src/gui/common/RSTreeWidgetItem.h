/****************************************************************
 * This file is distributed under the following license:
 *
 * Copyright (c) 2010, RetroShare Team
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


#ifndef _RSTREEWIDGETITEM_H
#define _RSTREEWIDGETITEM_H

#include <QTreeWidgetItem>

/* For definition of the UserRole for comparing */
class RSTreeWidgetItemCompareRole : QMap<int, int>
{
public:
    RSTreeWidgetItemCompareRole();

    void addRole(int column, int role);
    const int findRole(const int column) const;
};

class RSTreeWidgetItem : public QTreeWidgetItem
{
public:
    RSTreeWidgetItem(const RSTreeWidgetItemCompareRole *compareRole = NULL, int type = Type);
    RSTreeWidgetItem(const RSTreeWidgetItemCompareRole *compareRole, const QStringList &strings, int type = Type);
    RSTreeWidgetItem(const RSTreeWidgetItemCompareRole *compareRole, QTreeWidget *view, const QStringList &strings, int type = Type);
    RSTreeWidgetItem(const RSTreeWidgetItemCompareRole *compareRole, QTreeWidget *view, QTreeWidgetItem *after, int type = Type);
    RSTreeWidgetItem(const RSTreeWidgetItemCompareRole *compareRole, QTreeWidgetItem *parent, const QStringList &strings, int type = Type);
    RSTreeWidgetItem(const RSTreeWidgetItemCompareRole *compareRole, QTreeWidgetItem *parent, QTreeWidgetItem *after, int type = Type);
    RSTreeWidgetItem(const RSTreeWidgetItemCompareRole *compareRole, const QTreeWidgetItem &other);

    bool operator<(const QTreeWidgetItem &other) const;

private:
    const RSTreeWidgetItemCompareRole *m_compareRole;
};

#endif

