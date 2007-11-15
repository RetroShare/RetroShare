/*
 * "$Id: retrotray.h,v 1.5 2007-02-18 21:46:49 rmf24 Exp $"
 *
 * FltkGUI for RetroShare.
 *
 * Copyright 2004-2006 by Robert Fernie.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 *
 * Please report all bugs and problems to "retroshare@lunamutt.com".
 *
 */



#ifndef MRK_RETRO_TRAY_H
#define MRK_RETRO_TRAY_H

/**
 *
 * This class encapsulates the windows? system tray icon.
 *
 */

#include <windows.h>
#include <winsock2.h>
#include <process.h>
//#include <winbase.h>
//#include <winuser.h>
#include <shellapi.h>
#include <io.h>

// The Gui type of Window.
#include "fltkgui/guitab.h"

/**************** PQI_USE_XPGP ******************/
#if defined(PQI_USE_XPGP)

#include "pqi/xpgpcert.h"

#else /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/

#include "pqi/sslcert.h"

#endif /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/

int SetIconPath(const char *path);

int InitialiseRetroTray(HINSTANCE hi, HICON icn, UserInterface *ui);

class retroTray
{

public:
	retroTray();
int	init(HINSTANCE hi);
int	installIcon(HICON icn, UserInterface *ui);

int	buttonPressed(int, int);
private:		
void	showRetroShare() 
{
	if (getSSLRoot() -> active())
	{
		retroRoot -> main_win -> show();
		visible = true;
	}
	else
	{
		retroRoot -> welcome_window -> show();
	}
}

void	hideRetroShare() 
{
	retroRoot -> main_win -> hide();
	retroRoot -> welcome_window -> hide();
	retroRoot -> alert_window -> hide();
	retroRoot -> chatter_window -> hide();

	visible = false;
}

int	showTrayMenu();

	HINSTANCE hInst;
	HWND hWnd;
	HICON hIcon;
	NOTIFYICONDATA nid;
	UserInterface *retroRoot;
	bool visible;
};


#endif



