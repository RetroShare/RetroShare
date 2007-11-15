
/*
 * "$Id: testRsChanId.cc,v 1.1 2007-02-19 20:08:30 rmf24 Exp $"
 *
 * RetroShare C++ Interface.
 *
 * Copyright 2004-2007 by Robert Fernie.
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



#include "rstypes.h"
#include <iostream>
#include <sstream>
#include <string>

int main()
{
	RsCertId id;

	int i;
	for(i = 0; i < 16; i++)
	{
		id.data[i] = i+121;
	}

	std::cerr << "Cert Id: " << id << std::endl;

	std::ostringstream out;
	out << id;
	std::string idstr = out.str();

	std::cerr << "Cert Id (str): " << idstr << std::endl;

	RsCertId id2(idstr);

	std::cerr << "Cert Id2 (from str): " << id2 << std::endl;
	return 1;
}



