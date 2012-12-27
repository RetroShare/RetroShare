/*
 * libretroshare/src/upnp: upnpforward.cc
 *
 * Upnp Manual Test for RetroShare.
 *
 * Copyright 2012 by Robert Fernie.
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

/*
 * A Upnp utility to forward a port, specified on the commandline.
 */

#ifdef WIN32
#include "util/rswin.h"
#endif

#include "util/utest.h"
#include "upnp/upnphandler_linux.h"

INITTEST() ;

#include <sstream>

void usage(char *name)
{
	std::cerr << "Usage: " << name << " [-i <internal_port>] [-e <ext_port>]";
	std::cerr << std::endl;
}
	
int main(int argc, char **argv)
{
        int c;
        uint16_t internalPort = 7812;
        uint16_t externalPort = 7812;

#ifdef PTW32_STATIC_LIB
         pthread_win32_process_attach_np();
#endif 

#ifdef WIN32
        // Windows Networking Init.
        WORD wVerReq = MAKEWORD(2,2);
        WSADATA wsaData;
 
        if (0 != WSAStartup(wVerReq, &wsaData))
        {
                std::cerr << "Failed to Startup Windows Networking";
                std::cerr << std::endl;
        }
        else
        {
                std::cerr << "Started Windows Networking";
                std::cerr << std::endl;
        }

#endif


        while(-1 != (c = getopt(argc, argv, "i:e:")))
        {
                switch (c)
                {
                case 'i':
                        internalPort = atoi(optarg);
                        break;
                case 'e':
                        externalPort = atoi(optarg);
                        break;
                default:
                        usage(argv[0]);
                        break;
                }
        }


	/* create a upnphandler object */
	//pqiNetAssistFirewall *upnp;
	upnphandler *upnp;

	upnp = new upnphandler();

	/* setup */
	upnp->setInternalPort(internalPort);
	upnp->setExternalPort(externalPort);

	/* launch thread */
	upnp->enable(true);

	/* give it a chance to work its magic. */
	sleep(30);

	FINALREPORT("upnpforward") ;
	return TESTRESULT();
}



