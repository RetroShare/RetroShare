/*******************************************************************************
 * retroshare-gui/src/gui/gxs/GxsPerfProbe.h                                   *
 *                                                                             *
 * Copyright 2026 by Retroshare Team     <retroshare.project@gmail.com>        *
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

#pragma once

// Opt-in latency probes, off unless RS_GUI_PROFILE is set in the environment.
// Its value is a reporting threshold in milliseconds, 0 reports everything:
//
//     RS_GUI_PROFILE=0 ./retroshare
//
// Output goes through RsDbg(), i.e. stderr: launch from a terminal or the lines
// go nowhere.

#include <QString>
#include <QTimer>
#include <QCoreApplication>

#include <chrono>
#include <cstdlib>

#include "util/rsdebug.h"

namespace RsGuiPerf {

inline double threshold()
{
	// <0 means disabled. Read once, the environment does not change at runtime.
	static const double t = []() -> double {
		const char *v = getenv("RS_GUI_PROFILE");
		if(!v)
			v = getenv("RS_FORUM_PROFILE"); // legacy name of the first investigation
		return v ? atof(v) : -1.0;
	}();

	return t;
}

inline bool enabled() { return threshold() >= 0; }

/* Breadcrumb of the last operation a probe measured on the current thread. The
 * stall watchdog prints it, so a stall can be attributed even when the probe
 * that covered it stayed below the reporting threshold. */
inline const char *& lastOp()   { static thread_local const char *s = "none"; return s; }
inline double&       lastOpMs() { static thread_local double d = 0;           return d; }

/*!
 * \brief RAII timer around one operation.
 *
 * Report a probe placed on the GUI thread as time the interface stayed frozen.
 */
class Probe
{
public:
	explicit Probe(const char *what)
	    : mWhat(what), mStart(std::chrono::steady_clock::now()) {}

	~Probe()
	{
		if(!enabled())
			return;

		const double ms = std::chrono::duration<double,std::milli>(
		            std::chrono::steady_clock::now() - mStart ).count();

		lastOp()   = mWhat;
		lastOpMs() = ms;

		if(ms >= threshold())
			RsDbg() << "GUI-PROF " << mWhat << " " << mDetails.toStdString()
			        << " in " << ms << "ms";
	}

	void detail(const QString& s) { mDetails = s; }

private:
	const char *mWhat;
	QString mDetails;
	std::chrono::steady_clock::time_point mStart;
};

/*!
 * \brief Watchdog for the GUI thread itself.
 *
 * The probes above only measure the code they wrap, so they cannot see a stall
 * that happens anywhere else. This timer runs on the GUI thread and reports
 * whenever the event loop failed to come back on time, whatever the reason and
 * wherever the blocking code lives. It also names the last operation a probe
 * measured, which points at the culprit when one covers it.
 *
 * Safe to call several times, only the first call installs anything.
 */
inline void installGuiStallWatchdog()
{
	static bool installed = false;

	if(installed || !enabled())
		return;

	installed = true;

	static const int TICK_MS = 50;

	QTimer *timer = new QTimer(QCoreApplication::instance());
	auto *last = new std::chrono::steady_clock::time_point(
	            std::chrono::steady_clock::now() );

	QObject::connect(timer, &QTimer::timeout, QCoreApplication::instance(), [last]()
	{
		const auto now = std::chrono::steady_clock::now();
		const double ms = std::chrono::duration<double,std::milli>(now - *last).count();
		*last = now;

		// Anything above the tick plus a comfortable margin means the event loop
		// was busy or blocked for that long.
		if(ms > TICK_MS + 150)
			RsDbg() << "GUI-PROF GUI-THREAD-STALL " << (ms - TICK_MS)
			        << "ms  last_probe=" << lastOp() << " (" << lastOpMs() << "ms)";
	});

	timer->start(TICK_MS);
}

} // namespace RsGuiPerf
