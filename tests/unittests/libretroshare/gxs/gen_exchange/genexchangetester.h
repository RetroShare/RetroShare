/*******************************************************************************
 * unittests/libretroshare/gxs/gen_exchange/genexchangetester.h                *
 *                                                                             *
 * Copyright (C) 2018, Retroshare team <retroshare.team@gmailcom>              *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Lesser General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Lesser General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 ******************************************************************************/

#ifndef GENEXCHANGETESTER_H
#define GENEXCHANGETESTER_H

#include "genexchangetestservice.h"
#include "gxs/rsgds.h"
#include "gxs/rsnxs.h"

bool operator ==(const RsMsgMetaData& lMeta, const RsMsgMetaData& rMeta);
bool operator ==(const RsDummyMsg& lMsg, const RsDummyMsg& rMsg);
bool operator ==(const RsGxsGrpItem& lMsg, const RsGxsGrpItem& rMsg);

bool operator ==(const RsGroupMetaData& lMeta, const RsGroupMetaData& rMeta);
bool operator ==(const RsDummyGrp& lMsg, const RsDummyGrp& rMsg);
bool operator ==(const RsGxsMsgItem& lMsg, const RsGxsMsgItem& rMsg);

/*!
 * The idea of GenExchangeTest is to simplify test
 * of the RsGenExchange via the RsGxsDummyService
 * One can test all publish/request/meta-modify
 * capabilities of RsGenExchange
 * Simplifications comes from: \n
 *
 * - ability to store in and out data for comparison (in data are
 *   usually from requests, out data are from publications,
 *   but generally what you want to compare) \n
 * - convenience function to poll tokens \n
 *   - also allows filling in-data automatically from polls \n
 * - convenience interface for running tests
 */
class GenExchangeTest
{
public:

	/*!
	 * Constructs the GenExchangeTest with a tokenService
	 * @param tokenService This is needed. If not an instance of token service,
	 * behaviour of GenExchangeTest is undefined
	 */
	GenExchangeTest(GenExchangeTestService* const  mTestService, RsGeneralDataService* dataService, int pollingTO = 5 /* 5 secs default */);

	virtual ~GenExchangeTest();

	/*!
	 * This should be called in the main
	 * routines to execute all tests
	 * When implementing ensure units test header
	 * is in same file scope as implementation
	 * (you chould be using the CHECK functions
	 * to assert tests has passed)
	 */
	virtual void runTests() = 0;

protected:

    bool getServiceStatistic(const uint32_t &token, GxsServiceStatistic &servStatistic);
    bool getGroupStatistic(const uint32_t &token, GxsGroupStatistic &grpStatistic);

	/*!
	 * After each request and publish operation this should
	 * be called to ensure the operation has completed
	 * Requests will result in in data maps being filled
	 * @param
	 * @param opts
	 * @param fill if set to true, the received that is
	 *        routed to IN data structures
	 */
    void pollForToken(uint32_t token, const RsTokReqOptions& opts, bool fill = false);

    /*!
     * Allows to poll for token, and receive the message id
     * as acknowledgement. This function blocks for as long the
     * timeout value set on construction of tester
     * @param token
     * @param msgId
     */
    bool pollForMsgAcknowledgement(uint32_t token, RsGxsGrpMsgIdPair& msgId);

    /*!
	 * Allows to poll for token, and receive the group id
	 * as acknowledgement. This function blocks for as long the
	 * timeout value set on construction of tester
     * @param token
     * @param msgId
     */
    bool pollForGrpAcknowledgement(uint32_t token, RsGxsGroupId& msgId);

    GenExchangeTestService* getTestService();
    RsTokenService* getTokenService();
//    bool testGrpMetaModRequest();
//    bool testMsgMetaModRequest();

    // convenience functions for clearing IN and OUT data structures
    void clearMsgDataInMap();
    void clearMsgDataOutMap();
    void clearMsgMetaInMap();
    void clearMsgMetaOutMap();
    void clearMsgIdInMap();
    void clearMsgIdOutMap();
    void clearMsgRelatedIdInMap();
    void clearGrpDataInList();
    void clearGrpDataOutList();
    void clearGrpMetaInList();
    void clearGrpMetaOutList();
    void clearGrpIdInList();
    void clearGrpIdOutList();

    /*!
     * clears up all internal
     * IN and OUT data structure for
     * both msgs and groups
     * frees resources in relation to allocated data
     */
    void clearAllData();

    template <class Item>
    void deleteResVector(std::vector<Item*>& v) const
    {
    	typename std::vector<Item*>::iterator vit = v.begin();
    	for(; vit != v.end(); vit++)
    		delete *vit;
    	v.clear();
    }

    // using class to enable partial
	// function specialisation, bit of a hack in a
    // way
    template <class Cont, class Item>
    class Comparison
    {
    public:

		static bool comparison(const Cont& l, const Cont& r)
		{
			if(l.size() != r.size()) return false;

			typename Cont::const_iterator vit1 = l.begin(), vit2 = r.begin();

			while(vit1 != l.end())
			{
				const Item& item1 = (*vit1);
				const Item& item2 = (*vit2);
				if(!(item1 == item2)) return false;

				vit1++;
				vit2++;
			}
			return true;
		}
    };


    template <class Cont, class Item>
    class Comparison<Cont, Item*>
    {
    public:

		static bool comparison(const Cont& l, const Cont& r)
		{
			if(l.size() != r.size())
				return false;

			typename Cont::const_iterator vit1 = l.begin(), vit2 = r.begin();

			while(vit1 != l.end())
			{
				const Item* item1 = (*vit1);
				const Item* item2 = (*vit2);
				if(!(*item1 == *item2))
					return false;

				vit1++;
				vit2++;
			}
			return true;
		}
    };

    // convenience function for comparing IN and OUT data structures
    bool compareMsgDataMaps() ;
    bool compareMsgIdMaps() ;
    bool compareMsgMetaMaps() ;
    bool compareMsgRelateIdsMap() ;
    bool compareMsgRelatedDataMap() ;
    bool compareGrpData() ;
    bool compareGrpMeta() ;
    bool compareGrpIds() ;

    void storeToMsgDataOutMaps(const DummyMsgMap& msgDataOut);
    void storeToMsgIdsOutMaps(const GxsMsgIdResult& msgIdsOut);
    void storeToMsgMetaOutMaps(const GxsMsgMetaMap& msgMetaOut);

    void storeToMsgDataInMaps(const DummyMsgMap& msgDataOut);
	void storeToMsgIdsInMaps(const GxsMsgIdResult& msgIdsOut);
	void storeToMsgMetaInMaps(const GxsMsgMetaMap& msgMetaOut);

    void storeToGrpIdsOutList(const std::list<RsGxsGroupId>& grpIdOut);
    void storeToGrpMetaOutList(const std::list<RsGroupMetaData>& grpMetaOut);
    void storeToGrpDataOutList(const std::vector<RsDummyGrp*>& grpDataOut);

    void storeToGrpIdsInList(const std::list<RsGxsGroupId>& grpIdIn);
	void storeToGrpMetaInList(const std::list<RsGroupMetaData>& grpMetaOut);
	void storeToGrpDataInList(const std::vector<RsDummyGrp*>& grpDataOut);

    /*!
     * This sets up any resources required to operate a test
     */
    void setUp();

    /*!
     * Call at end of test to ensure resources
     * used in tests are released
     * This can invalidate other test runs if not called
     */
    void breakDown();

    /*!
     * initialises item to random data
     * @param grpItem item to initialise
     */
    void init(RsDummyGrp& grpItem) const;

    /*!
     * Initialises meta data to random data
     * @param grpMetaData
     */
    void init(RsGroupMetaData& grpMetaData) const;

    /*!
     * Initialises msg item to random data
     * @param msgItem
     */
    void init(RsDummyMsg& msgItem) const;

    /*!
     * Initialises meta data to random data
     * @param msgMetaData
     */
    void init(RsMsgMetaData& msgMetaData) const;


	void clearMsgDataMap(DummyMsgMap& msgDataMap) const;
	void clearGrpDataList(std::vector<RsDummyGrp*>& grpData) const;

    /*!
     * Helper function which sets up groups
     * data in to be used for publication
     * group data
     * @param nGrps number of groups to publish
     * @param groupId the ids for the created groups
     */
    void createGrps(uint32_t nGrps, std::list<RsGxsGroupId>& groupId);

    /*!
     * @return random number
     */
    uint32_t randNum() const;

private:

    std::vector<RsDummyGrp*> mGrpDataOut, mGrpDataIn;
    std::list<RsGroupMetaData> mGrpMetaDataOut, mGrpMetaDataIn;
    std::list<RsGxsGroupId> mGrpIdsOut, mGrpIdsIn;

    std::map<RsGxsGroupId, std::vector<RsDummyMsg*> > mMsgDataOut, mMsgDataIn;
    GxsMsgMetaMap mMsgMetaDataOut, mMsgMetaDataIn;
    GxsMsgIdResult mMsgIdsOut, mMsgIdsIn;

    MsgRelatedIdResult mMsgRelatedIdsOut, mMsgRelatedIdsIn;
    GxsMsgRelatedDataMap mMsgRelatedDataMapOut, mMsgRelatedDataMapIn;

    std::vector<RsGxsGroupId> mRandGrpIds; // ids that exist to help group testing

private:

    RsGeneralDataService* mDataService;
    GenExchangeTestService* mTestService;
    RsTokenService* mTokenService;
    int mPollingTO;
};

#endif // GENEXCHANGETESTER_H
