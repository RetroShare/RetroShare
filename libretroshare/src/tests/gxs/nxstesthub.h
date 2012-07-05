#ifndef NXSTESTHUB_H
#define NXSTESTHUB_H

#include "util/rsthreads.h"
#include "gxs/rsgxsnetservice.h"

/*!
 * This scenario module allows you to model
 * simply back and forth conversation between nxs and a virtual peer
 * (this module being the virtual peer)
 */
class NxsScenario
{

    static int SCENARIO_OUTGOING;
    static int SCENARIO_INCOMING;

public:

    virtual int scenarioType() = 0;

    virtual void receive(RsNxsItem* ) = 0;
    virtual RsNxsItem* send() = 0;

};



class NxsTestHub : public RsThread
{
public:


    NxsTestHub(NxsScenario* , std::pair<RsGxsNetService*, RsGxsNetService*> servicePair);

    /*!
     * To be called only after this thread has
     * been shutdown
     */
    bool testsPassed();

    void run();
};

#endif // NXSTESTHUB_H
