#include "rsgxsnetservice.h"

RsGxsNetService::RsGxsNetService(uint16_t servType,
                                 RsGeneralDataService *gds, RsNxsObserver *nxsObs)
                                     : mServType(servType), mDataStore(gds), mObserver(nxsObs)
{
}



void RsGxsNetService::tick(){


    if(receivedItems())
        recvNxsItemQueue();


}

void RsGxsNetService::recvNxsItemQueue(){

    RsItem *item ;

    while(NULL != (item=recvItem()))
    {
#ifdef NXS_NET_DEBUG
            std::cerr << "RsGxsNetService Item:" << (void*)item << std::endl ;
#endif
            // RsNxsItem needs dynamic_cast, since they have derived siblings.
            //
            RsNxsItem *ni = dynamic_cast<RsNxsItem*>(item) ;
            if(ni != NULL)
            {
                if(ni->transactionNumber != 0){

                    // accumulate
                    if(handleTransactionContent(ci))
                            delete ni ;

                    continue ;	// don't delete! It's handled by handleRecvChatMsgItem in some specific cases only.
                }


                switch(ni->PacketSubType())
                {
                case RS_PKT_SUBTYPE_NXS_SYNC_GRP: handleRecvSyncGroup (dynamic_cast<RsSyncGrp*>(ni)) ; break ;
                case RS_PKT_SUBTYPE_NXS_SYNC_MSG: handleRecvSyncMessage (dynamic_cast<RsSyncGrpMsg*>(ni)) ; break ;
                case RS_PKT_SUBTYPE_NXS_TRANS: handleRecvTransaction (dynamic_cast<RsNxsTransac*>(ni)) ; break;
                default:
                    std::cerr << "Unhandled item subtype " << ni->PacketSubType() << " in RsGxsNetService: " << std::endl;
                }
            delete item ;
        }
    }
}


}

void RsGxsNetService::run(){


    double timeDelta = 0.2;

    while(isRunning()){

#ifndef WINDOWS_SYS
        usleep((int) (timeDelta * 1000000));
#else
        Sleep((int) (timeDelta * 1000));
#endif

        processTransactions();

        processCompleteTransactions();

        processSyncRequests();
    }
}

void RsGxsNetService::recvItem(){

}





/** NxsTransaction definition **/


NxsTransaction::NxsTransaction()
    :mTransaction(NULL), mFlag(0), mTimestamp(0) {

}

NxsTransaction::~NxsTransaction(){

}
