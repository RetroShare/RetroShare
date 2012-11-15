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

#include <QDateTime>

#include "DateTime.h"
#include "rshare.h"

QString DateTime::formatLongDate(time_t dateValue)
{
	return formatLongDate(QDateTime::fromTime_t(dateValue).date());
}

QString DateTime::formatLongDate(const QDate &dateValue)
{
	QString customDateFormat = Rshare::customDateFormat();

	if (customDateFormat.isEmpty()) {
		return dateValue.toString(Qt::ISODate);
	} else {
		return QLocale().toString(dateValue, customDateFormat);
	}
}

QString DateTime::formatLongDateTime(time_t datetimeValue)
{
	return formatLongDateTime(QDateTime::fromTime_t(datetimeValue));
}

QString DateTime::formatLongDateTime(const QDateTime &datetimeValue)
{
	return formatLongDate(datetimeValue.date()) + " " + formatTime(datetimeValue.time());
}

QString DateTime::formatDateTime(time_t datetimeValue)
{
	return formatDateTime(QDateTime::fromTime_t(datetimeValue));
}

QString DateTime::formatDateTime(const QDateTime &datetimeValue)
{
	return formatDate(datetimeValue.date()) + " " + formatTime(datetimeValue.time());
}

QString DateTime::formatDate(time_t dateValue)
{
	return formatDate(QDateTime::fromTime_t(dateValue).date());
}

QString DateTime::formatDate(const QDate &dateValue)
{
	return dateValue.toString(Qt::SystemLocaleShortDate);
}

QString DateTime::formatTime(time_t timeValue)
{
	return formatTime(QDateTime::fromTime_t(timeValue).time());
}

QString DateTime::formatTime(const QTime &timeValue)
{
	return timeValue.toString(Qt::SystemLocaleShortDate);
}
