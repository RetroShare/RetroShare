#ifndef RETROSHARE_GUI_INTERFACE_H
#define RETROSHARE_GUI_INTERFACE_H

/*
 * "$Id: rsiface.h,v 1.9 2007-04-21 19:08:51 rmf24 Exp $"
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


#include "retroshare/rsnotify.h"
#include "rstypes.h"

#include <map>

class NotifyBase;
class RsServer;
class RsInit;
class RsPeerCryptoParams;
struct TurtleFileInfo ;

/* RsInit -> Configuration Parameters for RetroShare Startup
 */

RsInit *InitRsConfig();
/* extract various options for GUI */
const char   *RsConfigDirectory(RsInit *config);
bool    RsConfigStartMinimised(RsInit *config);
void    CleanupRsConfig(RsInit *);


// Called First... (handles comandline options) 
int InitRetroShare(int argc, char **argv, RsInit *config);

class RsControl /* The Main Interface Class - for controlling the server */
{
	public:
		static RsControl *instance() ;
		static void earlyInitNotificationSystem() { instance() ; }

		/* Real Startup Fn */
		virtual int StartupRetroShare() = 0;

		/****************************************/
		/* Config */

		virtual void    ConfigFinalSave( ) 			   = 0;
		virtual void 	rsGlobalShutDown( )			   = 0;

		/****************************************/

		virtual bool getPeerCryptoDetails(const RsPeerId& ssl_id,RsPeerCryptoParams& params) = 0;
		virtual void getLibraries(std::list<RsLibraryInfo> &libraries) = 0;

	protected:
		RsControl() {}	// should not be used, hence it's private.
};


#endif
