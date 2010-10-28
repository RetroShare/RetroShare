/*
 * libretroshare/src/tests/services distrib_services.h
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


#ifndef DISTRIB_SERVICE_TEST_H_
#define DISTRIB_SERVICE_TEST_H_

#include "Service_Test.h"

#include "services/p3forums.h"
#include "services/p3channels.h"
#include "services/p3blogs.h"

/*!
 * For testing forums
 */
class RsForum_Test : public RsServiceTest {

public:

	RsForum_Test();
	virtual ~RsForum_Test();

	/*!
	 * goes returns the result for given test cases
	 */
	void result(std::map<std::string, bool>&);

	bool setTestCases(std::list<std::string>& );
	bool runCases(std::list<std::string>&);

	/*!
	 * This is important for the get functions used by the deriving service
	 */
	void loadDummyData();

	const std::string TEST_CREATE_FORUMS;

private:

	bool testCreateForums();
	BIO *bio_err;
	p3Forums* forum;

};


#endif /* DISTRIB_SERVICE_TEST_H_ */
