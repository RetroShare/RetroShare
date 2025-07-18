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
	return formatLongDate(DateTimeFromTime_t(dateValue).date());
}

QString DateTime::formatLongDate(const QDate &dateValue)
{
	QString customDateFormat = RsApplication::customDateFormat();

	if (customDateFormat.isEmpty()) {
		return dateValue.toString(Qt::ISODate);
	} else {
		return QLocale().toString(dateValue, customDateFormat);
	}
}

QString DateTime::formatLongDateTime(time_t datetimeValue)
{
	return formatLongDateTime(DateTimeFromTime_t(datetimeValue));
}

QString DateTime::formatLongDateTime(const QDateTime &datetimeValue)
{
	return formatLongDate(datetimeValue.date()) + " " + formatTime(datetimeValue.time());
}

QString DateTime::formatDateTime(time_t datetimeValue)
{
	return formatDateTime(DateTimeFromTime_t(datetimeValue));
}

QString DateTime::formatDateTime(const QDateTime &datetimeValue)
{
	return formatDate(datetimeValue.date()) + " " + formatTime(datetimeValue.time());
}

QString DateTime::formatDate(time_t dateValue)
{
	return formatDate(DateTimeFromTime_t(dateValue).date());
}

QString DateTime::formatDate(const QDate &dateValue)
{
	return dateValue.toString(Qt::SystemLocaleShortDate);
}

QString DateTime::formatTime(time_t timeValue)
{
	return formatTime(DateTimeFromTime_t(timeValue).time());
}

QString DateTime::formatTime(const QTime &timeValue)
{
	return timeValue.toString(Qt::SystemLocaleShortDate);
}

QDateTime DateTime::DateTimeFromTime_t(time_t timeValue)
{
#if QT_VERSION >= QT_VERSION_CHECK (6, 0, 0)
	return QDateTime::fromSecsSinceEpoch(timeValue);
#else
	return QDateTime::fromTime_t(timeValue);
#endif
}
