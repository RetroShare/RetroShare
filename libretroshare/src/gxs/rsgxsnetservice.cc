#include "rsgxsnetservice.h"

#define SYNC_PERIOD 1000 // every 10 seconds
#define TRANSAC_TIMEOUT 10 // 10 seconds

RsGxsNetService::RsGxsNetService(uint16_t servType, RsGeneralDataService *gds,
                                 RsNxsNetMgr *netMgr, RsNxsObserver *nxsObs)
                                     : p3Config(servType), p3ThreadedService(servType), mServType(servType), mDataStore(gds),
                                       mObserver(nxsObs), mNxsMutex("RsGxsNetService"), mNetMgr(netMgr), mSYNC_PERIOD(SYNC_PERIOD)

{
	addSerialType(new RsNxsSerialiser(mServType));
}

RsGxsNetService::~RsGxsNetService()
{

}


int RsGxsNetService::tick(){

	// always check for new items arriving
	// from peers
    if(receivedItems())
        recvNxsItemQueue();

    uint32_t now = time(NULL);

    if((mSYNC_PERIOD + mSyncTs) < now)
    {
    	syncWithPeers();
    }

    return 1;
}

void RsGxsNetService::syncWithPeers()
{

	std::set<std::string> peers;
	mNetMgr->getOnlineList(peers);

	std::set<std::string>::iterator sit = peers.begin();

	// for now just grps
	for(; sit != peers.end(); sit++)
	{
		RsNxsSyncGrp *grp = new RsNxsSyncGrp(mServType);
		grp->PeerId(*sit);
		sendItem(grp);
	}

	// TODO msgs

}

bool RsGxsNetService::loadList(std::list<RsItem*>& load)
{
	return false;
}

bool RsGxsNetService::saveList(bool& cleanup, std::list<RsItem*>& save)
{
	return false;
}

RsSerialiser *RsGxsNetService::setupSerialiser()
{
	return NULL;
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

            	// a live transaction has a non zero value
                if(ni->transactionNumber != 0){

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
	 * If it does then check this not a initiating transactions
	 */

	RsStackMutex stack(mNxsMutex);

	const std::string& peer = item->PeerId();

	RsNxsTransac* transItem = dynamic_cast<RsNxsTransac*>(item);

	// if this is a RsNxsTransac item process
	if(transItem){
		return locked_processTransac(transItem);
	}

	// then this must be transaction content to be consumed
	// first check peer exist for transaction
	bool peerTransExists = mTransactions.find(peer) != mTransactions.end();

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

	/*!
	 * To process the transaction item
	 * It can either be initiating a transaction
	 * or ending one that already exists
	 *
	 * For initiating an incoming transaction the peer
	 * and transaction item need not exists
	 * as the peer will be added and transaction number
	 * added thereafter
	 *
	 * For commencing/starting an outgoing transaction
	 * the transaction must exist already
	 *
	 * For ending a transaction the
	 */

	const std::string& peer = item->PeerId();
	uint32_t transN = item->transactionNumber;
	NxsTransaction* tr = NULL;

	bool peerTrExists = mTransactions.find(peer) != mTransactions.end();
	bool transExists = false;

	if(peerTrExists){

		TransactionIdMap& transMap = mTransactions[peer];
		// record whether transaction exists already
		transExists = transMap.find(transN) != transMap.end();

	}

	// initiating an incoming transaction
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

		// note state as receiving, commencement item
		// is sent on next run() loop
		tr->mFlag = NxsTransaction::FLAG_STATE_STARTING;

		// commencement item for outgoing transaction
	}else if(item->transactFlag & RsNxsTransac::FLAG_BEGIN_P2){

		// transaction does not exist
		if(!peerTrExists || !transExists)
			return false;


		// alter state so transaction content is sent on
		// next run() loop
		TransactionIdMap& transMap = mTransactions[mOwnId];
		NxsTransaction* tr = transMap[transN];
		tr->mFlag = NxsTransaction::FLAG_STATE_SENDING;

		// end transac item for outgoing transaction
	}else if(item->transactFlag & RsNxsTransac::FLAG_END_SUCCESS){

		// transaction does not exist
		if(!peerTrExists || !transExists){
			return false;
		}

		// alter state so that transaction is removed
		// on next run() loop
		TransactionIdMap& transMap = mTransactions[mOwnId];
		NxsTransaction* tr = transMap[transN];
		tr->mFlag = NxsTransaction::FLAG_STATE_COMPLETED;
	}

	return true;
}

void RsGxsNetService::run(){


    double timeDelta = 0.2;

    while(isRunning()){

#ifndef WINDOWS_SYS
        usleep((int) (timeDelta * 1000000));
#else
        Sleep((int) (timeDelta * 1000));
#endif

        // process active transactions
        processTransactions();

        // process completed transactions
        processCompletedTransactions();

    }
}


void RsGxsNetService::processTransactions(){


	TransactionsPeerMap::iterator mit = mTransactions.begin();

	for(; mit != mTransactions.end(); mit++){

		TransactionIdMap& transMap = mit->second;
		TransactionIdMap::iterator mmit = transMap.begin(),

		mmit_end = transMap.end();

		/*!
		 * Transactions owned by peer
		 */
		if(mit->first == mOwnId){

			// transaction to be removed
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

					// if the number it item received equal that indicated
					// then transaction is marked as completed
					// to be moved to complete transations
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

					// unrecognised state
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

		// for a completed list response transaction
		// one needs generate requests from this
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
			// don't do anything, this should simply be removed

		}else if(flag & RsNxsTransac::FLAG_TYPE_GRPS)
		{

			std::list<RsNxsItem*>::iterator lit = tr->mItems.begin();
			std::vector<RsNxsGrp*> grps;

			while(tr->mItems.size() != 0)
			{
				RsNxsGrp* grp = dynamic_cast<RsNxsGrp*>(tr->mItems.front());

				if(grp)
				{
					tr->mItems.pop_front();
					grps.push_back(grp);
				}
				else
				{
#ifdef NXS_NET_DEBUG
					std::cerr << "RsGxsNetService::processCompletedTransactions(): item did not caste to grp"
							  << std::endl;
#endif
				}

			}

			// notify listener of grps
			mObserver->notifyNewGroups(grps);

		}else if(flag & RsNxsTransac::FLAG_TYPE_MSGS)
		{

			std::list<RsNxsItem*>::iterator lit = tr->mItems.begin();
			std::vector<RsNxsMsg*> msgs;

			while(tr->mItems.size() > 0)
			{
				RsNxsMsg* msg = dynamic_cast<RsNxsMsg*>(tr->mItems.front());
				if(msg)
				{
					tr->mItems.pop_front();
					msgs.push_back(msg);
				}
				else
				{
#ifdef NXS_NET_DEBUG
					std::cerr << "RsGxsNetService::processCompletedTransactions(): item did not caste to msg"
							  << std::endl;
#endif
				}
			}

			// notify listener of msgs
			mObserver->notifyNewMessages(msgs);

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

uint32_t RsGxsNetService::getTransactionId()
{
	RsStackMutex stack(mNxsMutex);

	return mTransactionN++;
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

void RsGxsNetService::handleRecvSyncGroup(RsNxsSyncGrp* item)
{

	std::string peer = item->PeerId();
	delete item;

	std::map<std::string, RsGxsGrpMetaData*> grp;
	mDataStore->retrieveGxsGrpMetaData(grp);

	if(grp.empty())
		return;

	std::vector<RsNxsSyncGrpItem*> grpSyncItems;
	std::map<std::string, RsGxsGrpMetaData*>::iterator mit =
	grp.begin();

	NxsTransaction* tr = new NxsTransaction();
	std::list<RsNxsItem*>& itemL = tr->mItems;

	for(; mit != grp.end(); mit++)
	{
		RsNxsSyncGrpItem* gItem = new
				RsNxsSyncGrpItem(mServType);
		gItem->flag = RsNxsSyncGrpItem::FLAG_RESPONSE;
		gItem->grpId = mit->first;
		gItem->publishTs = mit->second->mPublishTs;
		gItem->PeerId(peer);
		itemL.push_back(gItem);
	}

	tr->mFlag = NxsTransaction::FLAG_STATE_WAITING_CONFIRM;
	RsNxsTransac* trItem = new RsNxsTransac(mServType);
	trItem->transactFlag = RsNxsTransac::FLAG_BEGIN_P1
			| RsNxsTransac::FLAG_TYPE_GRP_LIST_RESP;
	trItem->nItems = itemL.size();
	time_t now;
	gmtime(&now);
	trItem->timeout = now + TRANSAC_TIMEOUT;
	trItem->PeerId(peer);
	trItem->transactionNumber = getTransactionId();

	// also make a copy for the resident transaction
	tr->mTransaction = new RsNxsTransac(*trItem);

	// signal peer to prepare for transaction
	sendItem(trItem);

	RsStackMutex stack(mNxsMutex);
	locked_addTransaction(tr);

	return;
}

void RsGxsNetService::handleRecvSyncMessage(RsNxsSyncMsg* item)
{
	return;
}


/** inherited methods **/

void RsGxsNetService::pauseSynchronisation(bool enabled)
{

}

void RsGxsNetService::setSyncAge(uint32_t age)
{

}

/** NxsTransaction definition **/

const uint8_t NxsTransaction::FLAG_STATE_STARTING = 0x0001; // when
const uint8_t NxsTransaction::FLAG_STATE_RECEIVING = 0x0002; // begin receiving items for incoming trans
const uint8_t NxsTransaction::FLAG_STATE_SENDING = 0x0004; // begin sending items for outgoing trans
const uint8_t NxsTransaction::FLAG_STATE_COMPLETED = 0x008;
const uint8_t NxsTransaction::FLAG_STATE_FAILED = 0x0010;
const uint8_t NxsTransaction::FLAG_STATE_WAITING_CONFIRM = 0x0020;


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
