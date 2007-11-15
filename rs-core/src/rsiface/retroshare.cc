
/*
 * "$Id: retroshare.cc,v 1.4 2007-04-21 19:08:51 rmf24 Exp $"
 *
 * RetroShare C++ Interface.
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


#include "rsiface/rsiface.h"   /* definition of iface */

#include "rsiface/notifytxt.h"

#include <iostream>
#ifdef WINDOWS_SYS
#include <winsock2.h>
#endif
                                

int main(int argc, char **argv)
{
	/* Objects */
	RsInit *config = InitRsConfig();
	InitRetroShare(argc, argv, config);
	LoadCertificates(config, false);

	//NotifyBase *notify = new NotifyBase();
	NotifyTxt *notify = new NotifyTxt();
	RsIface *iface = createRsIface(*notify);
        RsControl *rsServer = createRsControl(*iface, *notify);

	notify->setRsIface(iface);

	rsServer -> StartupRetroShare(config);
	CleanupRsConfig(config);
	
	/* pass control to the GUI */
	while(1)
	{
		std::cerr << "GUI Tick()" << std::endl;
#ifndef WINDOWS_SYS
		sleep(1);
#else
		Sleep(1000);
#endif
	}
	return 1;
}




