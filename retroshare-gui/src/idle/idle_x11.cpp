/*
 * idle_x11.cpp - detect desktop idle time
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

#ifndef HAVE_XSS

IdlePlatform::IdlePlatform() {}
IdlePlatform::~IdlePlatform() {}
bool IdlePlatform::init() { return false; }
int IdlePlatform::secondsIdle() { return 0; }

#else

#include <qapplication.h>
#include <QDesktopWidget>
#include <QX11Info>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/scrnsaver.h>

static XErrorHandler old_handler = 0;
extern "C" int xerrhandler(Display* dpy, XErrorEvent* err)
{
	if(err->error_code == BadDrawable)
		return 0;

	return (*old_handler)(dpy, err);
}

class IdlePlatform::Private
{
public:
	Private() {}

	XScreenSaverInfo *ss_info;
};

IdlePlatform::IdlePlatform()
{
	d = new Private;
	d->ss_info = 0;
}

IdlePlatform::~IdlePlatform()
{
	if(d->ss_info)
		XFree(d->ss_info);
	if(old_handler) {
		XSetErrorHandler(old_handler);
		old_handler = 0;
	}
	delete d;
}

bool IdlePlatform::init()
{
	if(d->ss_info)
		return true;

	old_handler = XSetErrorHandler(xerrhandler);

	int event_base, error_base;
	if(XScreenSaverQueryExtension(QApplication::desktop()->screen()->x11Display(), &event_base, &error_base)) {
		d->ss_info = XScreenSaverAllocInfo();
		return true;
	}
	return false;
}

int IdlePlatform::secondsIdle()
{
	if(!d->ss_info)
		return 0;
	if(!XScreenSaverQueryInfo(QApplication::desktop()->screen()->x11Display(), QX11Info::appRootWindow(), d->ss_info))
		return 0;
	return d->ss_info->idle / 1000;
}

#endif
