#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <util/rsid.h>
#include <pqi/p3linkmgr.h>
#include <pqi/authgpg.h>
#include <pqi/authssl.h>
#include <rsserver/p3peers.h>

class TestUtils
{
	public:
		// Creates a random file of the given size at the given place. Useful for file transfer tests.
		//
		static bool createRandomFile(const std::string& filename,const uint64_t size)
		{
			FILE *f = fopen(filename.c_str(),"wb") ;

			if(f == NULL)
				return 0 ;

			uint32_t S = 5000 ;
			uint32_t *data = new uint32_t[S] ;

			for(uint64_t i=0;i<size;i+=4*S)
			{
				for(uint32_t j=0;j<S;++j)
					data[j] = lrand48() ;

				uint32_t to_write = std::min((uint64_t)(4*S),size - i) ;

				if(to_write != fwrite((void*)data,1,to_write,f))
					return 0 ;
			}

			fclose(f) ;

			return true ;
		}

		static std::string createRandomSSLId()
		{
			return t_RsGenericIdType<16>::random().toStdString(false);
		}
		static std::string createRandomPGPId()
		{
			return t_RsGenericIdType<8>::random().toStdString(true);
		}

		class DummyAuthGPG: public AuthGPG
		{
			public:
				DummyAuthGPG(const std::string& ownId)
					:AuthGPG("pgp_pubring.pgp","pgp_secring.pgp","pgp_trustdb.pgp","lock"), mOwnId(ownId)
				{
				}

				virtual std::string getGPGOwnId() 
				{
					return mOwnId ;
				}

				virtual bool isGPGAccepted(const std::string& pgp_id) { return true ; }

			private:
				std::string mOwnId ;
		};

		class DummyAuthSSL: public AuthSSLimpl
		{
			public:
				DummyAuthSSL(const std::string& ownId)
					:	mOwnId(ownId)
				{
				}

				virtual std::string OwnId() 
				{
					return mOwnId ;
				}

			private:
				std::string mOwnId ;
		};

		class DummyRsPeers: public p3Peers
		{
			public:
				DummyRsPeers(p3LinkMgr *lm, p3PeerMgr *pm, p3NetMgr *nm) : p3Peers(lm,pm,nm) {}

				//virtual bool getFriendList(std::list<std::string>& fl) { ; return true ;}

			private:
				std::list<std::string> mFriends ;
		};
};
