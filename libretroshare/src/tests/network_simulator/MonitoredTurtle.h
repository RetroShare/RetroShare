#include <turtle/p3turtle.h>

class MonitoredTurtleRouter: public p3turtle
{
	public:
		MonitoredTurtleRouter(p3LinkMgr *lmgr,ftServer *fts)
			: p3turtle(lmgr,fts)
		{
		}

		// Overload functions that I don't want to be called for real!

		virtual bool loadConfiguration(std::string& loadHash) { return true ;}
		virtual bool saveConfiguration() { return true ;}
		virtual bool performLocalHashSearch(const TurtleFileHash& hash,FileInfo& info) ;

		// new functions to replace somme internal functionalities

		void provideFileHash(const std::string& hash) ;

	private:
		std::map<std::string,FileInfo> _local_files ;
};
