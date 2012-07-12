/*
 * nxstestscenario.h
 *
 *  Created on: 10 Jul 2012
 *      Author: crispy
 */

#ifndef NXSTESTSCENARIO_H_
#define NXSTESTSCENARIO_H_

#include <map>
#include "gxs/rsdataservice.h"
#include "gxs/rsnxsobserver.h"

/*!
 * This scenario module provides data resources
 */
class NxsTestScenario : public RsNxsObserver
{

public:

    virtual std::string getTestName() = 0;
    virtual RsGeneralDataService* dummyDataService1() = 0;
    virtual RsGeneralDataService* dummyDataService2() = 0;


};

class NxsMessageTest : public NxsTestScenario
{

public:

	NxsMessageTest();
	virtual ~NxsMessageTest();
    std::string getTestName();
    RsGeneralDataService* dummyDataService1();
    RsGeneralDataService* dummyDataService2();

public:

    /*!
     * @param messages messages are deleted after function returns
     */
    void notifyNewMessages(std::vector<RsNxsMsg*>& messages);

    /*!
     * @param messages messages are deleted after function returns
     */
    void notifyNewGroups(std::vector<RsNxsGrp*>& groups);


private:
    void setUpDataBases();
    void populateStore(RsGeneralDataService* dStore);

private:

    std::string mTestName;
    std::pair<RsGeneralDataService*, RsGeneralDataService*> mStorePair;
    std::map<std::string, std::vector<RsNxsMsg*> > mPeerMsgs;
    std::map<std::string, std::vector<RsNxsGrp*> > mPeerGrps;

};


#endif /* NXSTESTSCENARIO_H_ */
