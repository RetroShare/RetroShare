/*******************************************************************************
 * util/DateTime.h                                                             *
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

/* ... (header copyright) ... */

#ifndef _DATETIME_H
#define _DATETIME_H

#include <QString>

class QDateTime;
class QDate;
class QTime;

class DateTime
{
public:
	/* Always uses the standard Qt Long Format (e.g. "Tuesday, January 13, 2026") */
	static QString formatLongDate(time_t dateValue);
	static QString formatLongDate(const QDate &dateValue);

	/* Combined format: formatLongDate + formatTime */
	static QString formatLongDateTime(time_t datetimeValue);
	static QString formatLongDateTime(const QDateTime &datetimeValue);

	/* Uses Application Preferences if defined, otherwise falls back to System Short Format */
	static QString formatDate(time_t dateValue);
	static QString formatDate(const QDate &dateValue);

	/* Standard System Short Format for time */
	static QString formatTime(time_t timeValue);
	static QString formatTime(const QTime &timeValue);

	/* Combined format: formatDate + formatTime */
	static QString formatDateTime(time_t datetimeValue);
	static QString formatDateTime(const QDateTime &datetimeValue);

	/* Convert time_t to QDateTime */
	static QDateTime DateTimeFromTime_t(time_t timeValue);

	/* Convert QDateTime to time_t */
	static time_t DateTimeToTime_t(const QDateTime& dateTime);

	/*
	* Updates the internal cache for the date format.
	* Should be called at startup and whenever settings are changed.
	*/
	static void updateDateFormatCache();

private:
	static int mDateFormatCache;
	static int getDateFormat(); // Internal helper to get cached value

};

#endif
