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

#include "RSTreeWidgetItem.h"

RSTreeWidgetItemCompareRole::RSTreeWidgetItemCompareRole()
{
}

void RSTreeWidgetItemCompareRole::setRole(int column, int role)
{
    insert(column, role);
}

int RSTreeWidgetItemCompareRole::findRole(const int column) const
{
    RSTreeWidgetItemCompareRole::const_iterator it = find(column);
    if (it == end()) {
        return Qt::DisplayRole;
    }

    return it.value();
}

RSTreeWidgetItem::RSTreeWidgetItem(const RSTreeWidgetItemCompareRole *compareRole, int type) : QTreeWidgetItem(type)
{
    m_compareRole = compareRole;
}

RSTreeWidgetItem::RSTreeWidgetItem(const RSTreeWidgetItemCompareRole *compareRole, const QStringList &strings, int type) : QTreeWidgetItem(strings, type)
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
    int role = Qt::DisplayRole;

    /* Own role for sort defined ? */
    if (m_compareRole) {
        role = m_compareRole->findRole(column);
    }

    // taken from "bool QTreeWidgetItem::operator<(const QTreeWidgetItem &other) const"
    const QVariant v1 = data(column, role);
    const QVariant v2 = other.data(column, role);

    // taken from "bool QAbstractItemModelPrivate::variantLessThan(const QVariant &v1, const QVariant &v2)"
    switch(qMax(typeOfVariant(v1), typeOfVariant(v2)))
    {
    case 0: //integer type
        return v1.toLongLong() < v2.toLongLong();
    case 1: //floating point
        return v1.toDouble() < v2.toDouble();
    default:
        return (v1.toString().compare (v2.toString(), Qt::CaseInsensitive) < 0);
    }

    // let the standard do the sort, this code should not reached
    return QTreeWidgetItem::operator<(other);
}
