/****************************************************************
 * This file is distributed under the following license:
 *
 * Copyright (c) 2012, RetroShare Team
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

#include "RSListWidgetItem.h"

RSListWidgetItem::RSListWidgetItem(QListWidget *view, int type) : QListWidgetItem(view, type)
{
}

RSListWidgetItem::RSListWidgetItem(const QString &text, QListWidget *view, int type) : QListWidgetItem(text, view, type)
{
}

RSListWidgetItem::RSListWidgetItem(const QIcon &icon, const QString &text, QListWidget *view, int type) : QListWidgetItem(icon, text, view, type)
{
}

RSListWidgetItem::RSListWidgetItem(const QListWidgetItem &other) : QListWidgetItem(other)
{
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
	}
	return 2;
}

bool RSListWidgetItem::operator<(const QListWidgetItem &other) const
{
	const QVariant v1 = data(Qt::DisplayRole);
	const QVariant v2 = other.data(Qt::DisplayRole);

	switch (qMax(typeOfVariant(v1), typeOfVariant(v2))) {
	case 0: //integer type
		return v1.toLongLong() < v2.toLongLong();
	case 1: //floating point
		return v1.toReal() < v2.toReal();
	}

	return (v1.toString().compare (v2.toString(), Qt::CaseInsensitive) < 0);
}
