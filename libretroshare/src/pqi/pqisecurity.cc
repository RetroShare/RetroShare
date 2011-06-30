/*
 * "$Id: pqisecurity.cc,v 1.3 2007-02-18 21:46:49 rmf24 Exp $"
 *
 * 3P/PQI network interface for RetroShare.
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




#include "pqi/pqisecurity.h"
#include <stdlib.h>  // malloc


// Can keep the structure hidden....
// but won't at the moment.

// functions for checking what is allowed...
// currently these are all dummies.


std::string 		secpolicy_print(SecurityPolicy *)
{
	return std::string("secpolicy_print() Implement Me Please!");
}

SecurityPolicy *	secpolicy_create()
{
	return (SecurityPolicy *) malloc(sizeof(SecurityPolicy));
}

int	secpolicy_delete(SecurityPolicy *p)
{
	free(p);
	return 1;
}


int 			secpolicy_limit(SecurityPolicy *limiter,
				SecurityPolicy *alter)
{
	return 1;
}

int 			secpolicy_check(SecurityPolicy *, int type_transaction, 
						int direction)
{
	return 1;
}



