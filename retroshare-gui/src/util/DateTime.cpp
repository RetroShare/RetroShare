/*******************************************************************************
 * util/DateTime.cpp                                                           *
 *                                                                             *
 * Copyright (c) 2012 Retroshare Team  <retroshare.project@gmail.com>          *
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
