#include "rsgxsnetservice.h"

RsGxsNetService::RsGxsNetService(uint16_t servType,
                                 RsGeneralDataService *gds, RsNxsObserver *nxsObs)
                                     : p3Config(servType), mServType(servType), mDataStore(gds),
                                       mObserver(nxsObs), mNxsMutex("RsGxsNetService")

{
}



int RsGxsNetService::tick(){


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
                    if(handleTransaction(ni))
                            continue ;
                }


                switch(ni->PacketSubType())
                {
                case RS_PKT_SUBTYPE_NXS_SYNC_GRP: handleRecvSyncGroup (dynamic_cast<RsNxsSyncGrp*>(ni)) ; break ;
                case RS_PKT_SUBTYPE_NXS_SYNC_MSG: handleRecvSyncMessage (dynamic_cast<RsNxsSyncMsg*>(ni)) ; break ;
                default:
                    std::cerr << "Unhandled item subtype " << ni->PacketSubType() << " in RsGxsNetService: " << std::endl; break;
                }
            delete item ;
        }
    }
}


bool RsGxsNetService::handleTransaction(RsNxsItem* item){

	/*!
	 * This attempts to handle a transaction
	 * It first checks if this transaction id already exists
	 * If it does then create check this not a initiating transactions
	 */

	RsStackMutex stack(mNxsMutex);

	const std::string& peer = item->PeerId();

	RsNxsTransac* transItem = dynamic_cast<RsNxsTransac*>(item);

	// if this is an RsNxsTransac item process
	if(transItem){
		return locked_processTransac(transItem);
	}

	// then this must be transaction content to be consumed
	// first check peer exist for transaction
	bool peerTransExists = mTransactions.find(peer) == mTransactions.end();

	// then check transaction exists

	bool transExists = false;
	NxsTransaction* tr = NULL;
	uint32_t transN = item->transactionNumber;

	if(peerTransExists)
	{
		TransactionIdMap& transMap = mTransactions[peer];

		transExists = transMap.find(transN) != transMap.end();

		if(transExists){
			tr = transMap[transN];
			tr->mItems.push_back(item);
		}

	}else{
		return false;
	}

	return true;
}

bool RsGxsNetService::locked_processTransac(RsNxsTransac* item)
{

	const std::string& peer = item->PeerId();
	uint32_t transN = item->transactionNumber;
	NxsTransaction* tr = NULL;

	bool peerTrExists = mTransactions.find(peer) != mTransactions.end();
	bool transExists = false;

	if(peerTrExists){

		TransactionIdMap& transMap = mTransactions[peer];
		// remove current transaction if it does exist
		transExists = transMap.find(transN) != transMap.end();

	}

	if(item->transactFlag & RsNxsTransac::FLAG_BEGIN_P1){

		// create a transaction if the peer does not exist
		if(!peerTrExists){
			mTransactions[peer] = TransactionIdMap();
		}

		TransactionIdMap& transMap = mTransactions[peer];

		if(transExists)
			return false;

		// create new transaction
		tr = new NxsTransaction();
		transMap[transN] = tr;
		tr->mTransaction = item;
		tr->mTimestamp = time(NULL);

		// note state as receiving
		tr->mFlag = NxsTransaction::FLAG_STATE_STARTING;

	}else if(item->transactFlag & RsNxsTransac::FLAG_BEGIN_P2){

		// transaction does not exist
		if(!peerTrExists || !transExists)
			return false;


		// this means you need to start a transaction
		TransactionIdMap& transMap = mTransactions[mOwnId];
		NxsTransaction* tr = transMap[transN];
		tr->mFlag = NxsTransaction::FLAG_STATE_SENDING;

	}else if(item->transactFlag & RsNxsTransac::FLAG_END_SUCCESS){

		// transaction does not exist
		if(!peerTrExists || !transExists){
			return false;
		}

		// this means you need to start a transaction
		TransactionIdMap& transMap = mTransactions[mOwnId];
		NxsTransaction* tr = transMap[transN];
		tr->mFlag = NxsTransaction::FLAG_STATE_COMPLETED;

	}

	return false;
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

        processCompletedTransactions();

        processSyncRequests();
    }
}


void RsGxsNetService::processTransactions(){


	TransactionsPeerMap::iterator mit = mTransactions.begin();

	for(; mit != mTransactions.end(); mit++){

		TransactionIdMap& transMap = mit->second;
		TransactionIdMap::iterator mmit = transMap.begin(),

		mmit_end = transMap.end();

		if(mit->first == mOwnId){

			std::list<uint32_t> toRemove;

			for(; mmit != mmit_end; mmit++){

				NxsTransaction* tr = mmit->second;
				uint16_t flag = tr->mFlag;
				std::list<RsNxsItem*>::iterator lit, lit_end;
				uint32_t transN = tr->mTransaction->transactionNumber;

				// send items requested
				if(flag & NxsTransaction::FLAG_STATE_SENDING){

					lit = tr->mItems.begin();
					lit_end = tr->mItems.end();

					for(; lit != lit_end; lit++){
						sendItem(*lit);
					}

					tr->mItems.clear();
					tr->mFlag = NxsTransaction::FLAG_STATE_WAITING_CONFIRM;

				}else if(flag & NxsTransaction::FLAG_STATE_WAITING_CONFIRM){
					continue;

				}else if(flag & NxsTransaction::FLAG_STATE_COMPLETED){

					// move to completed transactions
					toRemove.push_back(transN);
					mComplTransactions.push_back(tr);
				}
			}

			std::list<uint32_t>::iterator lit = toRemove.begin();

			for(; lit != toRemove.end(); lit++)
			{
				transMap.erase(*lit);
			}

		}else{

			/*!
			 * Essentially these are incoming transactions
			 * Several states are dealth with
			 * Receiving: waiting to receive items from peer's transaction
			 * and checking if all have been received
			 * Completed: remove transaction from active and tell peer
			 * involved in transaction
			 * Starting: this is a new transaction and need to teell peer
			 * involved in transaction
			 */

			std::list<uint32_t> toRemove;

			for(; mmit != mmit_end; mmit++){

				NxsTransaction* tr = mmit->second;
				uint16_t flag = tr->mFlag;
				uint32_t transN = tr->mTransaction->transactionNumber;

				if(flag & NxsTransaction::FLAG_STATE_RECEIVING){
					// check if done
					if(tr->mItems.size() == tr->mTransaction->nItems)
						tr->mFlag = NxsTransaction::FLAG_STATE_COMPLETED;

				}else if(flag & NxsTransaction::FLAG_STATE_COMPLETED)
				{

					// send completion msg
					RsNxsTransac* trans = new RsNxsTransac(mServType);
					trans->clear();
					trans->transactFlag = RsNxsTransac::FLAG_END_SUCCESS;
					trans->transactionNumber = transN;
					trans->PeerId(tr->mTransaction->PeerId());
					sendItem(trans);

					// move to completed transactions
					mComplTransactions.push_back(tr);

					// transaction processing done
					// for this id, add to removal list
					toRemove.push_back(mmit->first);
				}else if(flag & NxsTransaction::FLAG_STATE_STARTING){

					// send item to tell peer your are ready to start
					RsNxsTransac* trans = new RsNxsTransac(mServType);
					trans->clear();
					trans->transactFlag = RsNxsTransac::FLAG_BEGIN_P2;
					trans->transactionNumber = transN;
					trans->PeerId(tr->mTransaction->PeerId());

				}
				else{

					// if no state
					std::cerr << "RsGxsNetService::processTransactions() Unrecognised statem, deleting " << std::endl;
					std::cerr << "RsGxsNetService::processTransactions() Id: "
							  << transN << std::endl;

					toRemove.push_back(transN);
				}
			}

			std::list<uint32_t>::iterator lit = toRemove.begin();

			for(; lit != toRemove.end(); lit++)
			{
				transMap.erase(*lit);
			}
		}
	}
}

void RsGxsNetService::processCompletedTransactions()
{
	/*!
	 * Depending on transaction we may have to respond to peer
	 * responsible for transaction
	 */
	std::list<NxsTransaction*>::iterator lit = mComplTransactions.begin();

	while(mComplTransactions.size()>0)
	{

		NxsTransaction* tr = mComplTransactions.front();

		uint16_t flag = tr->mTransaction->transactFlag;

		if(flag & RsNxsTransac::FLAG_TYPE_MSG_LIST_RESP)
		{
			// generate request based on a peers response
			genReqMsgTransaction(tr);

		}else if(flag & RsNxsTransac::FLAG_TYPE_GRP_LIST_RESP)
		{
			genReqGrpTransaction(tr);
		}
		else if( (flag & RsNxsTransac::FLAG_TYPE_MSG_LIST_REQ) ||
				(flag & RsNxsTransac::FLAG_TYPE_GRP_LIST_REQ) )
		{
			// don't do anything

		}else if(flag & RsNxsTransac::FLAG_TYPE_GRPS)
		{

			std::list<RsNxsItem*>::iterator lit = tr->mItems.begin();
			std::list<RsNxsGrp*> grps;

			while(tr->mItems.size() != 0)
			{
				RsNxsGrp* grp = dynamic_cast<RsNxsGrp*>(tr->mItems.front());

				if(grp)
					tr->mItems.pop_front();
				else
				{
#ifdef NXS_NET_DEBUG
					std::cerr << "RsGxsNetService::processCompletedTransactions(): item did not caste to grp"
							  << std::endl;
#endif
				}

			}

			// notify listener of grps
			notifyListenerGrps(grps);

		}else if(flag & RsNxsTransac::FLAG_TYPE_MSGS)
		{

			std::list<RsNxsItem*>::iterator lit = tr->mItems.begin();
			std::list<RsNxsMsg*> msgs;

			while(tr->mItems.size() > 0)
			{
				RsNxsMsg* msg = dynamic_cast<RsNxsMsg*>(tr->mItems.front());
				if(msg)
				{
					tr->mItems.pop_front();
				}else
				{
#ifdef NXS_NET_DEBUG
					std::cerr << "RsGxsNetService::processCompletedTransactions(): item did not caste to msg"
							  << std::endl;
#endif
				}
			}

			// notify listener of msgs
			notifyListenerMsgs(msgs);
		}

		delete tr;
		mComplTransactions.pop_front();
	}
}


void RsGxsNetService::genReqMsgTransaction(NxsTransaction* tr)
{

	// to create a transaction you need to know who you are transacting with
	// then what msgs to request
	// then add an active Transaction for request

	std::list<RsNxsSyncMsgItem*> msgItemL;

	std::list<RsNxsItem*>::iterator lit = tr->mItems.begin();

        // first get item list sent from transaction
	for(; lit != tr->mItems.end(); lit++)
	{
		RsNxsSyncMsgItem* item = dynamic_cast<RsNxsSyncMsgItem*>(*lit);
		if(item)
		{
			msgItemL.push_back(item);
		}else
		{
#ifdef NXS_NET_DEBUG
                        std::cerr << "RsGxsNetService::genReqMsgTransaction(): item failed cast to RsNxsSyncMsgItem* "
					  << std::endl;
#endif
			delete item;
			item = NULL;
		}
	}


        // get grp id for this transaction
        RsNxsSyncMsgItem* item = msgItemL.front();
        const std::string& grpId = item->grpId;
        std::vector<std::string> grpIdV;
        grpIdV.push_back(grpId);
        GxsMsgMetaResult result;
        mDataStore->retrieveGxsMsgMetaData(grpIdV, result);
        std::vector<RsGxsMsgMetaData*> &msgMetaV = result[grpId];

        std::vector<RsGxsMsgMetaData*>::const_iterator vit = msgMetaV.begin();
        std::set<std::string> msgIdSet;

        // put ids in set for each searching
        for(; vit != msgMetaV.end(); vit++)
            msgIdSet.insert((*vit)->mMsgId);

        // get unique id for this transaction
	uint32_t transN = getTransactionId();


        // now do compare and add loop
        std::list<RsNxsSyncMsgItem*>::iterator llit = msgItemL.begin();
        std::list<RsNxsItem*> reqList;

	for(; llit != msgItemL.end(); llit++)
	{
		const std::string& msgId = (*llit)->msgId;

                if(msgIdSet.find(msgId) == msgIdSet.end()){
			RsNxsSyncMsgItem* msgItem = new RsNxsSyncMsgItem(mServType);
			msgItem->grpId = grpId;
			msgItem->msgId = msgId;
			msgItem->flag = RsNxsSyncMsgItem::FLAG_REQUEST;
			msgItem->transactionNumber = transN;
			reqList.push_back(msgItem);
		}
	}


	RsNxsTransac* transac = new RsNxsTransac(mServType);
	transac->transactFlag = RsNxsTransac::FLAG_TYPE_MSG_LIST_REQ
			| RsNxsTransac::FLAG_BEGIN_P1;
	transac->timeout = mTransactionTimeOut;
	transac->nItems = reqList.size();

	NxsTransaction* newTrans = new NxsTransaction();
	newTrans->mItems = reqList;
	newTrans->mFlag = NxsTransaction::FLAG_STATE_STARTING;
	newTrans->mTimestamp = 0;
	newTrans->mTransaction = transac;
	{
		RsStackMutex stack(mNxsMutex);
		if(!locked_addTransaction(newTrans))
			delete newTrans;
	}
}

void RsGxsNetService::genReqGrpTransaction(NxsTransaction* tr)
{

	// to create a transaction you need to know who you are transacting with
	// then what grps to request
	// then add an active Transaction for request

	std::list<RsNxsSyncGrpItem*> grpItemL;

	std::list<RsNxsItem*>::iterator lit = tr->mItems.begin();

	for(; lit != tr->mItems.end(); lit++)
	{
		RsNxsSyncGrpItem* item = dynamic_cast<RsNxsSyncGrpItem*>(*lit);
		if(item)
		{
			grpItemL.push_back(item);
		}else
		{
#ifdef NXS_NET_DEBUG
			std::cerr << "RsGxsNetService::genReqMsgTransaction(): item failed to caste to RsNxsSyncMsgItem* "
					  << std::endl;
#endif
			delete item;
			item = NULL;
		}
	}

	RsNxsSyncGrpItem* item = grpItemL.front();
	const std::string& grpId = item->grpId;
        std::map<std::string, RsGxsGrpMetaData*> grpMetaMap;
        mDataStore->retrieveGxsGrpMetaData(grpMetaMap);

	// now do compare and add loop
	std::list<RsNxsSyncGrpItem*>::iterator llit = grpItemL.begin();
	std::list<RsNxsItem*> reqList;

	uint32_t transN = getTransactionId();

	for(; llit != grpItemL.end(); llit++)
	{
		const std::string& grpId = (*llit)->grpId;

                if(grpMetaMap.find(grpId) == grpMetaMap.end()){
			RsNxsSyncGrpItem* grpItem = new RsNxsSyncGrpItem(mServType);

			grpItem->grpId = grpId;
			grpItem->flag = RsNxsSyncMsgItem::FLAG_REQUEST;
			grpItem->transactionNumber = transN;
			reqList.push_back(grpItem);
		}
	}


	RsNxsTransac* transac = new RsNxsTransac(mServType);
	transac->transactFlag = RsNxsTransac::FLAG_TYPE_MSG_LIST_REQ
			| RsNxsTransac::FLAG_BEGIN_P1;
	transac->timeout = mTransactionTimeOut;
	transac->nItems = reqList.size();

	NxsTransaction* newTrans = new NxsTransaction();
	newTrans->mItems = reqList;
	newTrans->mFlag = NxsTransaction::FLAG_STATE_STARTING;
	newTrans->mTimestamp = 0;
	newTrans->mTransaction = transac;

	{
		RsStackMutex stack(mNxsMutex);
		if(!locked_addTransaction(newTrans))
			delete newTrans;
	}
}

bool RsGxsNetService::locked_addTransaction(NxsTransaction* tr)
{
	const std::string& peer = tr->mTransaction->PeerId();
	uint32_t transN = tr->mTransaction->transactionNumber;
	TransactionIdMap& transMap = mTransactions[peer];
	bool transNumExist = transMap.find(transN)
			!= transMap.end();


	if(transNumExist){
		return false;
	}else{
		transMap[transN] = tr;
		return true;
	}
}

void RsGxsNetService::cleanTransactionItems(NxsTransaction* tr) const
{

	std::list<RsNxsItem*>::iterator lit = tr->mItems.begin();

	for(; lit != tr->mItems.end(); lit++)
	{
		delete *lit;
	}

	tr->mItems.clear();
}

/** inherited methods **/

void RsGxsNetService::pauseSynchronisation(bool enabled)
{

}

void RsGxsNetService::setSyncAge(uint32_t age)
{

}

/** NxsTransaction definition **/

NxsTransaction::NxsTransaction()
    : mFlag(0), mTimestamp(0), mTransaction(NULL) {

}

NxsTransaction::~NxsTransaction(){

	std::list<RsNxsItem*>::iterator lit = mItems.begin();

	for(; lit != mItems.end(); lit++)
	{
		delete *lit;
		*lit = NULL;
	}

	delete mTransaction;
	mTransaction = NULL;
}
