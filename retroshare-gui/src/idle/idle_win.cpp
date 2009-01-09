/*
 * idle_win.cpp - detect desktop idle time
 * Copyright (C) 2003  Justin Karneges
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
#include "idle.h"

#include <QLibrary>
#include <windows.h>

#ifndef tagLASTINPUTINFO
typedef struct __tagLASTINPUTINFO {
	UINT cbSize;
	DWORD dwTime;
 } __LASTINPUTINFO, *__PLASTINPUTINFO;
#endif

class IdlePlatform::Private
{
public:
	Private()
	{
		GetLastInputInfo = NULL;
		lib = 0;
	}

	BOOL (__stdcall * GetLastInputInfo)(__PLASTINPUTINFO);
	DWORD (__stdcall * IdleUIGetLastInputTime)(void);
	QLibrary *lib;
};

IdlePlatform::IdlePlatform()
{
	d = new Private;
}

IdlePlatform::~IdlePlatform()
{
	delete d->lib;
	delete d;
}

bool IdlePlatform::init()
{
	if(d->lib)
		return true;
	void *p;

	// try to find the built-in Windows 2000 function
	d->lib = new QLibrary("user32");
	if(d->lib->load() && (p = d->lib->resolve("GetLastInputInfo"))) {
		d->GetLastInputInfo = (BOOL (__stdcall *)(__PLASTINPUTINFO))p;
		return true;
	} else {
		delete d->lib;
		d->lib = 0;
	}

	// fall back on idleui
	d->lib = new QLibrary("idleui");
	if(d->lib->load() && (p = d->lib->resolve("IdleUIGetLastInputTime"))) {
		d->IdleUIGetLastInputTime = (DWORD (__stdcall *)(void))p;
		return true;
	} else {
		delete d->lib;
		d->lib = 0;
	}

	return false;
}

int IdlePlatform::secondsIdle()
{
	int i;
	if(d->GetLastInputInfo) {
		__LASTINPUTINFO li;
		li.cbSize = sizeof(__LASTINPUTINFO);
		bool ok = d->GetLastInputInfo(&li);
		if(!ok)
			return 0;
		i = li.dwTime;
	} else if (d->IdleUIGetLastInputTime) {
		i = d->IdleUIGetLastInputTime();
	} else
		return 0;

	return (GetTickCount() - i) / 1000;
}
