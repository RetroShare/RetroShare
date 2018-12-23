/*******************************************************************************
 * gui/common/RSTreeWidgetItem.h                                               *
 *                                                                             *
 * Copyright (C) 2010 RetroShare Team <retroshare.project@gmail.com>           *
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

#ifndef _RSTREEWIDGETITEM_H
#define _RSTREEWIDGETITEM_H

#include <QMap>
#include <QList>
#include <QTreeWidgetItem>

/* For definition of the UserRole for comparing */
class RSTreeWidgetItemCompareRole : QMap<int, QList<int> >
{
public:
    RSTreeWidgetItemCompareRole();

    void setRole(int column, int role);
    void addRole(int column, int role);
    void findRoles(const int column, QList<int> &roles) const;
};

class RSTreeWidgetItem : public QTreeWidgetItem
{
public:
    RSTreeWidgetItem(const RSTreeWidgetItemCompareRole *compareRole = NULL, int type = Type);
    RSTreeWidgetItem(const RSTreeWidgetItemCompareRole *compareRole, const QStringList &strings, int type = Type);
    RSTreeWidgetItem(const RSTreeWidgetItemCompareRole *compareRole, QTreeWidget *view, int type = Type);
    RSTreeWidgetItem(const RSTreeWidgetItemCompareRole *compareRole, QTreeWidget *view, const QStringList &strings, int type = Type);
    RSTreeWidgetItem(const RSTreeWidgetItemCompareRole *compareRole, QTreeWidget *view, QTreeWidgetItem *after, int type = Type);
    RSTreeWidgetItem(const RSTreeWidgetItemCompareRole *compareRole, QTreeWidgetItem *parent, int type = Type);
    RSTreeWidgetItem(const RSTreeWidgetItemCompareRole *compareRole, QTreeWidgetItem *parent, const QStringList &strings, int type = Type);
    RSTreeWidgetItem(const RSTreeWidgetItemCompareRole *compareRole, QTreeWidgetItem *parent, QTreeWidgetItem *after, int type = Type);
    RSTreeWidgetItem(const RSTreeWidgetItemCompareRole *compareRole, const QTreeWidgetItem &other);

    bool operator<(const QTreeWidgetItem &other) const;

private:
    const RSTreeWidgetItemCompareRole *m_compareRole;
};

#endif

