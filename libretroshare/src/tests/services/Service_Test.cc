/*
 * libretroshare/src/tests/services Service_Test.cc
 *
 * RetroShare Service Testing
 *
 * Copyright 2010 by Chris Evi-Parker.
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

#include "Service_Test.h"


ServiceTestFrame::ServiceTestFrame()
{


}

ServiceTestFrame::~ServiceTestFrame()
{

}

bool ServiceTestFrame::addTest(const std::string& testName, RsServiceTest* st)
{

	if((st == NULL) || (testName.empty()))
		return false;

	serviceTests.insert(std::pair<std::string, RsServiceTest* >(testName, st));
	return true;

}


void ServiceTestFrame::BeginTesting()
{
	std::cout << "Begining Testing" << std::endl;

	return;
}

void ServiceTestFrame::FinishTesting()
{

	std::cout << "End of Testing\n Printing Results" << std::endl;

	return;
}

/***************************** RsServiceTest *******************/


RsServiceTest::RsServiceTest()
{

}

RsServiceTest::~RsServiceTest()
{


}


void RsServiceTest::result(std::map<std::string, bool>& results){

	std::map<std::string, bool>::iterator mit = testResults.begin();

	std::cout << "Results of the Test : " << std::endl;
	std::string passed("Passed"), failed("Failed");
	for(;mit != testResults.end();  mit++){

		std::cout << mit->first << mit->second?passed:failed;
//				  << std::endl;
	}

	std::cout << "End of Results" << std::endl;

	return;
}

