/*******************************************************************************
 * gui/common/RSTreeWidgetItem.cpp                                             *
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

#include "RSTreeWidgetItem.h"

RSTreeWidgetItemCompareRole::RSTreeWidgetItemCompareRole()
{
}

void RSTreeWidgetItemCompareRole::setRole(int column, int role)
{
    QList<int> roles;
    roles.push_back(role);
    insert(column, roles);
}

void RSTreeWidgetItemCompareRole::addRole(int column, int role)
{
    RSTreeWidgetItemCompareRole::iterator it = find(column);
    if (it == end()) {
        setRole(column, role);
        return;
    }

    it.value().push_back(role);
}

void RSTreeWidgetItemCompareRole::findRoles(const int column, QList<int> &roles) const
{
    RSTreeWidgetItemCompareRole::const_iterator it = find(column);
    if (it == end()) {
        roles.clear();
        roles.push_back(Qt::DisplayRole);
        return;
    }

    roles = it.value();
}

RSTreeWidgetItem::RSTreeWidgetItem(const RSTreeWidgetItemCompareRole *compareRole, int type) : QTreeWidgetItem(type)
{
    m_compareRole = compareRole;
}

RSTreeWidgetItem::RSTreeWidgetItem(const RSTreeWidgetItemCompareRole *compareRole, const QStringList &strings, int type) : QTreeWidgetItem(strings, type)
{
    m_compareRole = compareRole;
}

RSTreeWidgetItem::RSTreeWidgetItem(const RSTreeWidgetItemCompareRole *compareRole, QTreeWidget *view, int type) : QTreeWidgetItem(view, type)
{
    m_compareRole = compareRole;
}

RSTreeWidgetItem::RSTreeWidgetItem(const RSTreeWidgetItemCompareRole *compareRole, QTreeWidget *view, const QStringList &strings, int type) : QTreeWidgetItem(view, strings, type)
{
    m_compareRole = compareRole;
}

RSTreeWidgetItem::RSTreeWidgetItem(const RSTreeWidgetItemCompareRole *compareRole, QTreeWidget *view, QTreeWidgetItem *after, int type) : QTreeWidgetItem(view, after, type)
{
    m_compareRole = compareRole;
}

RSTreeWidgetItem::RSTreeWidgetItem(const RSTreeWidgetItemCompareRole *compareRole, QTreeWidgetItem *parent, int type) : QTreeWidgetItem(parent, type)
{
    m_compareRole = compareRole;
}

RSTreeWidgetItem::RSTreeWidgetItem(const RSTreeWidgetItemCompareRole *compareRole, QTreeWidgetItem *parent, const QStringList &strings, int type) : QTreeWidgetItem(parent, strings, type)
{
    m_compareRole = compareRole;
}

RSTreeWidgetItem::RSTreeWidgetItem(const RSTreeWidgetItemCompareRole *compareRole, QTreeWidgetItem *parent, QTreeWidgetItem *after, int type) : QTreeWidgetItem(parent, after, type)
{
    m_compareRole = compareRole;
}

RSTreeWidgetItem::RSTreeWidgetItem(const RSTreeWidgetItemCompareRole *compareRole, const QTreeWidgetItem &other) : QTreeWidgetItem(other)
{
    m_compareRole = compareRole;
}

static uint typeOfVariant(const QVariant &value)
{
    //return 0 for integer, 1 for floating point and 2 for other
    switch (value.userType()) {
        case QVariant::Bool:
        case QVariant::Int:
        case QVariant::UInt:
        case QVariant::LongLong:
        case QVariant::ULongLong:
        case QVariant::Char:
        case QMetaType::Short:
        case QMetaType::UShort:
        case QMetaType::UChar:
        case QMetaType::ULong:
        case QMetaType::Long:
            return 0;
        case QVariant::Double:
        case QMetaType::Float:
            return 1;
        default:
            return 2;
    }
}

bool RSTreeWidgetItem::operator<(const QTreeWidgetItem &other) const
{
    int column = treeWidget()->sortColumn();
    QList<int> roles;

    /* Own role for sort defined ? */
    if (m_compareRole) {
        m_compareRole->findRoles(column, roles);
    }

    QList<int>::iterator role;
    for (role = roles.begin(); role != roles.end(); ++role) {
        // taken from "bool QTreeWidgetItem::operator<(const QTreeWidgetItem &other) const"
        const QVariant v1 = data(column, *role);
        const QVariant v2 = other.data(column, *role);

        // taken from "bool QAbstractItemModelPrivate::variantLessThan(const QVariant &v1, const QVariant &v2)"
        switch(qMax(typeOfVariant(v1), typeOfVariant(v2)))
        {
        case 0: //integer type
            {
                qlonglong value1 = v1.toLongLong();
                qlonglong value2 = v2.toLongLong();

                if (value1 != value2) {
                    return value1 < value2;
                }
            }
            break;
        case 1: //floating point
            {
                double value1 = v1.toDouble();
                double value2 = v2.toDouble();

                if (value1 != value2) {
                    return value1 < value2;
                }
            }
            break;
        default:
            {
                int compare = v1.toString().compare (v2.toString(), Qt::CaseInsensitive);
                if (compare) {
                    return (compare < 0);
                }
            }
        }
    }

    /* Compare DisplayRole */
    const QVariant v1 = data(column, Qt::DisplayRole);
    const QVariant v2 = other.data(column, Qt::DisplayRole);

    return (v1.toString().compare (v2.toString(), Qt::CaseInsensitive) < 0);
}
