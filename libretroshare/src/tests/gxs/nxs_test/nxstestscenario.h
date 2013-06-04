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
class NxsTestScenario
{

public:

    virtual std::string getTestName() = 0;

    /*!
     * @param peer
     * @param namePath
     * @return data service with populated with random grp/msg data, null if peer or pathname exists
     */
    virtual RsGeneralDataService* getDataService(const std::string& peer) = 0;


    virtual bool testPassed() = 0;
    /*!
     * Service type for this test
     * should correspond to serialiser service type
     */
    virtual uint16_t getServiceType() = 0;

    /*!
     * Call to remove files created
     * in the test directory
     */
    virtual void cleanUp() = 0;


};

class NxsMessageTestObserver : public RsNxsObserver
{
public:

    NxsMessageTestObserver(RsGeneralDataService* dStore);

    /*!
     * @param messages messages are deleted after function returns
     */
    void notifyNewMessages(std::vector<RsNxsMsg*>& messages);

    /*!
     * @param messages messages are deleted after function returns
     */
    void notifyNewGroups(std::vector<RsNxsGrp*>& groups);

private:

    RsGeneralDataService* mStore;

};

class NxsMessageTest : public NxsTestScenario
{

public:

    NxsMessageTest(uint16_t servtype);
    virtual ~NxsMessageTest();
    std::string getTestName();
    uint16_t getServiceType();
    RsGeneralDataService* getDataService(const std::string& peer);

    /*!
     * Call to remove files created
     * in the test directory
     */
    void cleanUp();

    bool testPassed();

private:
    void setUpDataBases();
    void populateStore(RsGeneralDataService* dStore);

private:

    std::string mTestName;
    std::map<std::string, RsGeneralDataService*> mPeerStoreMap;
    std::set<std::string> mStoreNames;
    uint16_t mServType;

    RsMutex mMsgTestMtx;

};


#endif /* NXSTESTSCENARIO_H_ */
