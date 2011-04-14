/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2006,2007 crypton
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
 
#ifndef _EVENTRECEIVER_H
#define _EVENTRECEIVER_H

#include <QWidget>
#include <QSharedMemory>

class QUrl;

class EventReceiver : public
#ifdef WINDOWS_SYS
	QWidget
#else
	QObject
#endif
{
	Q_OBJECT

public:
	EventReceiver();
	~EventReceiver();

	bool start();
	bool sendRetroShareLink(const QString& link);

signals:
	void linkReceived(const QUrl& url);

private:
	void received(const QString& url);

#ifdef WINDOWS_SYS
	/* Extend QWidget with a class that will capture the WM_COPYDATA messages */
	bool winEvent (MSG* message, long* result);
#endif // WINDOWS_SYS

	QSharedMemory sharedMMemory;
};

#endif
