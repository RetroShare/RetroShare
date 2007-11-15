/*
 * "$Id: pqisecurity.h,v 1.3 2007-02-18 21:46:49 rmf24 Exp $"
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



#ifndef MRK_PQI_SECURITY_HEADER
#define MRK_PQI_SECURITY_HEADER

#include "pqi/pqi.h"
#include "pqi/pqipacket.h"

#define PQI_INCOMING 2
#define PQI_OUTGOING 5

//structure.
typedef struct sec_policy
{
     int searchable; // flags indicate how searchable we are..
} SecurityPolicy;

// functions for checking what is allowed...
// 

std::string 		secpolicy_print(SecurityPolicy *);
SecurityPolicy *	secpolicy_create();
int			secpolicy_delete(SecurityPolicy *);
int 			secpolicy_limit(SecurityPolicy *limiter,
				SecurityPolicy *alter);
int 			secpolicy_check(SecurityPolicy *, int type_transaction, 
						int direction);


#endif

