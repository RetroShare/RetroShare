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

#include "servicetest.h"
#include "services/p3forums.h"

class forumServiceTest : public ServiceTest {

public:

	forumServiceTest();
	~forumServiceTest();


	void runTests();

private:

	bool getForumMsgTest();
	bool setForumMsgTest();
	bool createGroupTests();

private:

	void loadDummyData();

private:

	p3Forums* mForums;

};


#endif /* DISTRIB_SERVICE_TEST_H_ */
