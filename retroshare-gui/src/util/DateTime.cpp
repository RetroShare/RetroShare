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
#include <QLocale>

#include "DateTime.h"
#include "rshare.h"
#include "gui/settings/rsharesettings.h"

/*
 * This utility class provides standardized date and time formatting across the application.
 * - formatDate() and formatTime(): These methods dynamically adjust the output 
 * based on the user's selected preference in the application settings 
 * (System = Qt Short Format, ISO 8601, Text = Qt Long Format)
 * - formatLongDate() and formatLongDateTime(): use standard Qt Long Format
 */

// Initialiaze cache
int DateTime::mDateFormatCache = -1;


// Cache management
void DateTime::updateDateFormatCache()
{
    mDateFormatCache = Settings->getDateFormat();
}

int DateTime::getDateFormat()
{
    if (mDateFormatCache == -1) {
        updateDateFormatCache();
    }
    return mDateFormatCache;
}

// --- Date Functions ---

QString DateTime::formatDate(time_t dateValue)
{
	return formatDate(DateTimeFromTime_t(dateValue).date());
}

QString DateTime::formatDate(const QDate &dateValue)
{
	/* Retrieve the date format index from global settings */
	int dateFormat = getDateFormat(); 

	if (dateFormat == RshareSettings::DateFormat_ISO) {
		/* Option "ISO 8601": Returns YYYY-MM-DD */
		return dateValue.toString(Qt::ISODate);
	} 
	
	if (dateFormat == RshareSettings::DateFormat_Text) {
		/* Option "Text": Returns the system's Long Format */
		return QLocale::system().toString(dateValue, QLocale::LongFormat);
	}

	/* Default or "System" option: Returns the system's Short Format */
	return QLocale::system().toString(dateValue, QLocale::ShortFormat);
}

// --- Long Date Functions ---

QString DateTime::formatLongDate(time_t dateValue)
{
	return formatLongDate(DateTimeFromTime_t(dateValue).date());
}

QString DateTime::formatLongDate(const QDate &dateValue)
{
	// Strictly use the system's long format (descriptive)
	return QLocale::system().toString(dateValue, QLocale::LongFormat);
}

QString DateTime::formatLongDateTime(time_t datetimeValue)
{
	return formatLongDateTime(DateTimeFromTime_t(datetimeValue));
}

QString DateTime::formatLongDateTime(const QDateTime &datetimeValue)
{
	return formatLongDate(datetimeValue.date()) + " " + formatTime(datetimeValue.time());
}

// --- Time Functions ---

QString DateTime::formatTime(time_t timeValue)
{
	return formatTime(DateTimeFromTime_t(timeValue).time());
}

QString DateTime::formatTime(const QTime &timeValue)
{
	int dateFormat = getDateFormat();

	if (dateFormat == RshareSettings::DateFormat_ISO) {
		/* ISO standard implies 24h format (HH:mm) */
		return timeValue.toString("HH:mm");
	}

	/* Default or "System" option: Returns the system's Short Format (respects 12h/24h locale) */
	return QLocale::system().toString(timeValue, QLocale::ShortFormat);
}

// --- Combined Functions ---

QString DateTime::formatDateTime(time_t datetimeValue)
{
	return formatDateTime(DateTimeFromTime_t(datetimeValue));
}

QString DateTime::formatDateTime(const QDateTime &datetimeValue)
{
	/* Combines the date (respecting user preference) with the system time */
	return formatDate(datetimeValue.date()) + " " + formatTime(datetimeValue.time());
}

// --- Conversions Functions ---

QDateTime DateTime::DateTimeFromTime_t(time_t timeValue)
{
#if QT_VERSION >= QT_VERSION_CHECK (6, 0, 0)
	return QDateTime::fromSecsSinceEpoch(timeValue);
#else
	return QDateTime::fromTime_t(timeValue);
#endif
}

time_t DateTime::DateTimeToTime_t(const QDateTime& dateTime)
{
#if QT_VERSION >= QT_VERSION_CHECK (6, 0, 0)
	return dateTime.toSecsSinceEpoch();
#else
	return dateTime.toTime_t();
#endif
}
