/*
 * "$Id: retrotray.cc,v 1.5 2007-02-18 21:46:49 rmf24 Exp $"
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




#include <windows.h>
#include <iostream>
#include <stdlib.h>

#include "fltkgui/retroTray.h"
#include "pqi/pqidebug.h"

static retroTray tray;
static NOTIFYICONDATA s_nid;
static std::string icn_path;


int SetIconPath(const char *iconpath)
{
	icn_path = iconpath;
	return 1;
}


int InitialiseRetroTray(HINSTANCE hi, HICON icn, UserInterface *ui)
{
	tray.init(hi);

	std::string iconpath = "/programs/icons";
	std::string fname = "tst.ico";

	// for now install a default one.
//	if (icn == (HICON) NULL)
	{
		//icn = LoadCursorFromFile(fname.c_str());
		icn = LoadCursorFromFile(icn_path.c_str());
		//icn = (HICON) LoadImage(hi, fname.c_str(), IMAGE_ICON, 16, 16, 0);
		std::cerr << "Loaded ICON: " << icn << std::endl;
	}

	//tray.loadIcons(iconpath);
	tray.installIcon(icn, ui);

//	while(1)
//		Sleep(1000);
	return 1;
}




static LRESULT CALLBACK systray_handler(HWND hwnd, UINT msg, WPARAM type0, LPARAM type) {
	static UINT taskbarRestartMsg; /* static here means value is kept across multiple calls to this func */


	std::cerr << "Call of The Handler(" << msg << ",";
	std::cerr << type0 << "," << type << ")" << std::endl;

	switch(msg) {
	case WM_CREATE:
		std::cerr << "WM_CREATE";
		//taskbarRestartMsg = RegisterWindowMessage("TaskbarCreated");
		break;
		
	case WM_TIMER:
		std::cerr << "WM_TIMER";
		break;

	case WM_DESTROY:
		std::cerr << "WM_DESTROY";
		break;

	case WM_USER:
	{
		std::cerr << "WM_USER";
		tray.buttonPressed(type0, type);
		break;
	}
	default: 
		std::cerr << "default";
		if (msg == taskbarRestartMsg) 
		{
			std::cerr << " RESTART MSG";
			//tray.InstallIcon(0);
		}
		break;
	}

	std::cerr << std::endl;
	//return CallDefWindowProc(hwnd, msg, type0, type);
	return CallWindowProc(DefWindowProc, hwnd, msg, type0, type);
}


retroTray::retroTray()
	:hInst(0)
{
	//ZeroMemory(&hwin, sizeof(hwin));
	ZeroMemory(&nid, sizeof(nid));
	return;
}


int retroTray::init(HINSTANCE hi)
{
	hInst = hi;
	return 1;
}


int retroTray::installIcon(HICON ic, UserInterface *ui)
{
	retroRoot = ui;
	hIcon = ic;
//(HICON) IDI_QUESTION; 
//icn;

	// First create hidden window (could be RetroShare)
	if (nid.cbSize!=sizeof(NOTIFYICONDATA))
	{
		WNDCLASSEX wcex;
		wcex.style = 0;
		wcex.cbClsExtra = 0;
		wcex.cbWndExtra = 0;
		wcex.hIcon = NULL;
		wcex.hIconSm = NULL;
		wcex.hCursor = NULL;
		wcex.hbrBackground = NULL;
		wcex.lpszMenuName = NULL;
		wcex.lpszClassName = "retroTray";
	
		wcex.hInstance = hInst;
		wcex.lpfnWndProc = (WNDPROC) systray_handler;
		wcex.cbSize = sizeof(WNDCLASSEX);
	
		RegisterClassEx(&wcex);
	
		hWnd = CreateWindow("retroTray", "", 0,0,0,0,0,
			GetDesktopWindow(), NULL, hInst, 0);
	
		std::cerr << "Created Hidden Window" << std::endl;
	}

	ZeroMemory(&nid, sizeof(nid));
	nid.hWnd = hWnd;
	nid.uID = 0; //nextid++;
	nid.uFlags= NIF_ICON | NIF_MESSAGE | NIF_TIP;
	nid.uCallbackMessage=WM_USER;
	nid.hIcon=hIcon;

	strcpy(nid.szTip, "Hello World from RetroShare");
	nid.cbSize=sizeof(NOTIFYICONDATA);

	s_nid = nid;

	if (Shell_NotifyIcon(NIM_ADD, &s_nid))
	{
		std::cerr << "Notify Success" << std::endl;
	}
	else
	{
		std::cerr << "Notify Failure" << std::endl;
	}

	std::cerr << "Icon Installed??" << std::endl;
	return 1;
}

int retroTray::buttonPressed(int type0, int type)
{
	std::cerr << "retroTray::buttonPressed(" << type0;
	std::cerr << "," << type << ")" << std::endl;

	if (type == WM_LBUTTONDBLCLK)
	{
		std::cerr << "Showing RetroShare!" << std::endl;
		showRetroShare();
	}
	else if (type == WM_LBUTTONUP)
	{
		std::cerr << "Showing RetroShare(B)!" << std::endl;
		showRetroShare();
	}
	else if (type == WM_MBUTTONUP)
	{
		std::cerr << "Hiding RetroShare!" << std::endl;
		hideRetroShare();
	}
        else if (type == WM_RBUTTONDOWN)
	{
		std::cerr << "Showing TaskMenu!" << std::endl;
		showTrayMenu();
	}
	else //if (type == WM_RBUTTONUP)
	{
		std::cerr << "Leaving RetroShare!" << std::endl;
		// do nothing.
	}	
	return 1;
}

//
//int retroTray::addIcon()
//{
//	return 1;
//}
//
//
//int retroTray::removeIcon()
//{
//	return 1;
//}
//
//
//int retroTray::exitCall()
//{
//	// if 
//static  int times = 0;
//	if (++times > 20)
//		removeIcon();
//	return 1;
//}
//
//
//



#define RTRAY_MENU_NULL 0
#define RTRAY_MENU_HIDE 1
#define RTRAY_MENU_SHOW  2
#define RTRAY_MENU_QUIT  3


int retroTray::showTrayMenu(void)
{
    long   menuChoice;

// First We need a Menu.
        HMENU popup;
	unsigned int flag;

        popup = ::CreatePopupMenu();

	flag = MF_BYPOSITION;
	::InsertMenu(popup, 0, flag, RTRAY_MENU_QUIT, "Quit");
	flag = MF_BYPOSITION | MF_SEPARATOR;
	::InsertMenu(popup, 0, flag, 0x000, "Seperator");
	flag = MF_BYPOSITION;
	::InsertMenu(popup, 0, flag, RTRAY_MENU_HIDE, "Hide RetroShare");
	::InsertMenu(popup, 0, flag, RTRAY_MENU_SHOW, "Open RetroShare");

	flag = MF_BYPOSITION | MF_SEPARATOR;
	::InsertMenu(popup, 0, flag, 0x000, "Seperator");
	flag = MF_BYPOSITION;
	::InsertMenu(popup, 0, flag, 0x001, "About RetroShare");

	POINT pp;

	RECT rect;
	rect.left = 0;
	rect.top = 0;
	rect.right = 1000;
	rect.bottom = 1000;

    	::GetCursorPos(&pp);
    	menuChoice	= ::TrackPopupMenu(popup,
                           TPM_LEFTALIGN | 
                           TPM_LEFTBUTTON | 
                           TPM_RIGHTBUTTON | 
                           TPM_NONOTIFY | 
                           TPM_RETURNCMD,
                           pp.x,pp.y, 0, hWnd, &rect);
    	::DestroyMenu(popup);
        
    // The value of SelectionMade is the id of the command selected or 0 if no 
    // selection was made
        

    switch(menuChoice)
    {
        case RTRAY_MENU_NULL:
/*
            AfxMessageBox("Starting Null");
*/
	    break;

        case RTRAY_MENU_HIDE:
		hideRetroShare();
	    break;
        case RTRAY_MENU_SHOW:
		showRetroShare();
            break;

        case RTRAY_MENU_QUIT:
	    /* before exiting - clear the debug log */
	    clearDebugCrashLog();
	    exit(1);
            break;
    }
    return 1;
}


/**********************
HWND CreateDialog(      

    HINSTANCE hInstance,
    LPCTSTR lpTemplate,
    HWND hWndParent,
    DLGPROC lpDialogFunc
);

**********************/

