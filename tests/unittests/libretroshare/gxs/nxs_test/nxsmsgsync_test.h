/*
 * nxsmsgsync_test.h
 *
 *  Created on: 13 Apr 2014
 *      Author: crispy
 */

#ifndef NXSMSGSYNC_TEST_H_
#define NXSMSGSYNC_TEST_H_




	class NxsMessageTest : public NxsTestScenario
	{

	public:

		NxsMessageTest(uint16_t servtype);
		virtual ~NxsMessageTest();
		std::string getTestName();
		uint16_t getServiceType();
		RsGeneralDataService* getDataService(const RsPeerId& peer);

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
		std::map<RsPeerId, RsGeneralDataService*> mPeerStoreMap;
		std::set<std::string> mStoreNames;
		uint16_t mServType;

		RsMutex mMsgTestMtx;

	};




#endif /* NXSMSGSYNC_TEST_H_ */
