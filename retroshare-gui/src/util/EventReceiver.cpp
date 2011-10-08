/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2006, 2007 crypton
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

#include <QCoreApplication>
#include <QMessageBox>
#include <QIcon>
#include <iostream>

#include <retroshare/rsinit.h>

#include "EventReceiver.h"
#include "gui/RetroShareLink.h"

#ifdef WINDOWS_SYS
#include <windows.h>
#define OP_RETROSHARELINK 12000
#endif

struct SharedMemoryInfo
{
#ifdef WINDOWS_SYS
	/* Store handle of the event window */
	WId wid;
#else
	long dummy;
#endif
};

EventReceiver::EventReceiver()
{
#ifdef WINDOWS_SYS
	setWindowTitle("RetroShare EventReceiver");
#endif

	/* Build unique name for the running instance */
	QString name = QString("RetroShare-%1::EventReceiver").arg(QCoreApplication::applicationDirPath());
	sharedMemory.setKey(name);
}

EventReceiver::~EventReceiver()
{
	sharedMemory.detach();
}

bool EventReceiver::start()
{
	if (!sharedMemory.create(sizeof(SharedMemoryInfo))) {
		std::cerr << "EventReceiver::start() Cannot create shared memory !" << sharedMemory.errorString().toStdString() << std::endl;
		return false;
	}

	bool result = true;

	if (sharedMemory.lock()) {
		SharedMemoryInfo *info = (SharedMemoryInfo*) sharedMemory.data();
		if (info) {
#ifdef WINDOWS_SYS
			info->wid = winId();
#else
			info->dummy = 0;
#endif
		} else {
			result = false;
			std::cerr << "EventReceiver::start() Shared memory returns a NULL pointer!" << std::endl;
		}

		sharedMemory.unlock();
	} else {
		result = false;
		std::cerr << "EventReceiver::start() Cannot lock shared memory !" << std::endl;
	}

	return result;
}

bool EventReceiver::sendRetroShareLink(const QString& link)
{
	if (!sharedMemory.attach()) {
		/* No running instance found */
		return false;
	}

	bool result = true;

	if (sharedMemory.lock()) {
		SharedMemoryInfo *info = (SharedMemoryInfo*) sharedMemory.data();
		if (info) {
#ifdef WINDOWS_SYS
			if (info->wid) {
				QByteArray linkData(link.toUtf8());

				COPYDATASTRUCT send;
				send.dwData = OP_RETROSHARELINK;
				send.cbData = link.length() * sizeof(char);
				send.lpData = (void*) linkData.constData();

				SendMessage((HWND) info->wid, WM_COPYDATA, (WPARAM) 0, (LPARAM) (PCOPYDATASTRUCT) &send);
			} else {
				result = false;
			}
#else
			Q_UNUSED(link);

			QMessageBox mb(QMessageBox::Critical, "RetroShare", QObject::tr("Start with a RetroShare link is only supported for Windows."), QMessageBox::Ok);
			mb.setWindowIcon(QIcon(":/images/rstray3.png"));
			mb.exec();

			result = false;
#endif
		} else {
			result = false;
			std::cerr << "EventReceiver::sendRetroShareLink() Cannot lock shared memory !" << std::endl;
		}

		sharedMemory.unlock();
	} else {
		result = false;
		std::cerr << "EventReceiver::start() Cannot lock shared memory !" << std::endl;
	}

	sharedMemory.detach();

	return result;
}

#ifdef WINDOWS_SYS
bool EventReceiver::winEvent(MSG* message, long* result)
{
	if (message->message == WM_COPYDATA ) {
		/* Extract the struct from lParam */
		COPYDATASTRUCT *data = (COPYDATASTRUCT*) message->lParam;

		if (data && data->dwData == OP_RETROSHARELINK) {
			received(QString::fromUtf8((const char*) data->lpData, data->cbData));

			/* Keep the event from Qt */
			*result = 0;
			return true;
		}
	}

	/* Give the event to Qt */
	return false;
}
#endif

void EventReceiver::received(const QString &url)
{
	RetroShareLink link(url);
	if (link.valid()) {
		emit linkReceived(link.toUrl());
	}
}
