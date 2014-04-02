#include <turtle/p3turtle.h>

class MonitoredTurtleRouter: public p3turtle
{
	public:
		MonitoredTurtleRouter(p3ServiceControl *sc,p3LinkMgr *lmgr,ftServer *fts)
			: p3turtle(sc,lmgr)
		{
		}

		// Overload functions that I don't want to be called for real!

		virtual bool loadConfiguration(RsFileHash& loadHash) { return true ;}
		virtual bool saveConfiguration() { return true ;}
		virtual bool performLocalHashSearch(const TurtleFileHash& hash,const RsPeerId& peer_id,FileInfo& info) ;

		// new functions to replace somme internal functionalities

		void provideFileHash(const RsFileHash& hash) ;

	private:
		std::map<RsFileHash,FileInfo> _local_files ;
};
