/*
 * libretroshare/src/tests/services Service_Test.h
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


#ifndef SERVICE_TEST_H_
#define SERVICE_TEST_H_

#include <dbase/cachestrapper.h>
#include <string>
#include <list>
#include <map>


class RsServiceTest;


/*!
 * This class deals with actual RsServiceTest objects and the share resources between these objects.
 * It has the ability to run the tests
 * held by service test objects, and also holds shared resources that are used by the test objects (networking, disk access)
 */
class ServiceTestFrame {

public:
	ServiceTestFrame();
	~ServiceTestFrame();


	/*!
	 * This adds to the list of test to be done by retroshare
	 * @testName a name for this test
	 * @RsServiceTest_Frame a handle to the service test object
	 */
	bool addTest(const std::string& testName, RsServiceTest* );

	/*!
	 * Begin testing
	 */
	void BeginTesting();

	/*!
	 * prints out results of tests
	 */
	void FinishTesting();


private:

	std::map<std::string, RsServiceTest* > serviceTests;

	/* resources need by the RsServiceTestObjects */

	CacheStrapper *cs;
	CacheTransfer *cft;
};

/*!
 * The aim of this class is form the basis for how services are
 * tested
 */
class RsServiceTest {

public:

	RsServiceTest();
	~RsServiceTest();

	/*!
	 * goes returns the result for given test cases
	 */
	void result(std::map<std::string, bool>&);
	virtual bool setTestCases(std::list<std::string>& ) = 0;
	virtual bool runCases(std::list<std::string>&) = 0;

	/*!
	 * This is important for the get functions used by the deriving service
	 */
	virtual void loadDummyData() = 0;


private:

	/// contains the test id to determine which set of tests are to be run
	std::list<std::string> testCase;

	/// maps to the results of test
	std::map<std::string, bool> testResults;
};

#endif /* SERVICE_TEST_H_ */
