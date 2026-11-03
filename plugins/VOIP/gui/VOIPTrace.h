/*******************************************************************************
 * plugins/VOIP/gui/VOIPTrace.h                                                *
 *                                                                             *
 * Copyright (C) 2026 by Retroshare Team <retroshare.project@gmail.com>        *
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

// Debug trace for the audio/video setup. Each line is written to stderr and appended to
//     <temp dir>/retroshare_voip_trace.log
// (on Windows: %TEMP%\retroshare_voip_trace.log), and the file is flushed after every line,
// so that the last line tells where the program was if it freezes.
//
// Disabled by default: set VOIP_TRACE_ENABLED to 1 (here or with -DVOIP_TRACE_ENABLED=1) to debug.

#ifndef VOIP_TRACE_ENABLED
#define VOIP_TRACE_ENABLED 0
#endif

#include <QString>

#if VOIP_TRACE_ENABLED
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <iostream>

inline void voipTrace(const QString& msg)
{
    std::cerr << "[VOIP] " << msg.toStdString() << std::endl;

    QFile f(QDir::temp().filePath("retroshare_voip_trace.log"));
    if(f.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
    {
        QTextStream ts(&f);
        ts << QDateTime::currentDateTime().toString("hh:mm:ss.zzz") << "  " << msg << "\n";
        ts.flush();
        f.close();
    }
}
#define VOIP_TRACE(msg) voipTrace(msg)
#else
#define VOIP_TRACE(msg) do {} while(0)
#endif
