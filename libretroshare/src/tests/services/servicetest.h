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

#include "pqi/p3connmgr.h"
#include "rsserver/p3peers.h"
#include "dbase/cachestrapper.h"
#include "pqi/pqipersongrp.h"




/*!
 * A convenience class from which tests derive from
 * This enables user to test in shallow manner the public methods
 * of a service
 */
class ServiceTest {

public:
	ServiceTest();
	virtual ~ServiceTest();

	/*!
	 * all tests of service should be implemented here
	 */
    virtual void runTests() = 0;

    /*!
     * use this to populate the service with messages
     */
    void sendItem(RsItem* item);

protected:


    p3ConnectMgr* mConnMgr;
	CacheStrapper *mCs;
	CacheTransfer *mCt;
	p3Peers* mPeers;
	pqipersongrp* mPersonGrp;

	std::string fakePeer; // ssl id of fake receiving peer

};



#endif /* SERVICE_TEST_H_ */
