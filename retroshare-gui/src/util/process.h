/****************************************************************
 * This file is distributed under the following license:
 *
 * Copyright (c) 2006-2007, crypton
 * Copyright (c) 2006, Matt Edman, Justin Hipple
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


#ifndef _PROCESS_H
#define _PROCESS_H

#include <QString>

#if defined(Q_OS_WIN)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#endif


/** Returns the PID of the current process. */
qint64 get_pid();

/** Returns true if a process with the given PID is running. */
bool is_process_running(qint64 pid);

/** Writes the given file to disk containing the current process's PID. */
bool write_pidfile(QString pidfile, QString *errmsg = 0);

/** Reads the giiven pidfile and returns the value in it. If the file does not
 * exist, -1 is returned. */
qint64 read_pidfile(QString pidfile, QString *errmsg = 0);

#endif

