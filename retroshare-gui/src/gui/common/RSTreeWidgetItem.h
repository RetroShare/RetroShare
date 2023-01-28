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
	explicit RSTreeWidgetItemCompareRole(QMap<int, QList<int>> map);

    void clear() { QMap<int, QList<int> >::clear() ; }
	void setRole(const int column, const int role);
	void addRole(const int column, const int role);
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

	bool noOrder() const {return m_noOrder;}
	///
	/// \brief setNoOrder: Disabled normal sort for this item. It use column 0 and role defined to sort in ascending in qlonglong format.
	/// \param value: True disable normal sort but use defined role.
	/// \param useRole: The role to use to ascending sort in first(0) column.
	///
	void setNoOrder(const bool value, const int useRole = Qt::UserRole) {m_noOrder = value; m_useRoleWhenNoOrder = useRole;}

	bool noDataAsLast() const {return m_noDataAsLast;}
	///
	/// \brief setNoDataAsLast: Item without data will be sort as last. Need a RSTreeWidgetItemCompareRole to get it activated.
	/// \param value: True to enable this role.
	///
	void setNoDataAsLast(const bool value) {m_noDataAsLast = value;}

private:
	const RSTreeWidgetItemCompareRole *m_compareRole;
	bool m_noOrder;
	int m_useRoleWhenNoOrder;
	bool m_noDataAsLast;
};

#endif

