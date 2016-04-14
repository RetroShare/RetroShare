// This class crawls the given directry hierarchy and updates it. It does so by calling the
// shared file list source. This source may be of two types:
// 	- local: directories are crawled n disk and files are hashed / requested from a cache
// 	- remote: directories are requested remotely to a providing client
//
class DirectoryUpdater
{
	public:
		DirectoryUpdater() ;

		// Does some updating job. Crawls the existing directories and checks wether it has been updated
		// recently enough. If not, calls the directry source.
		//
		void tick() ;

		// 
};

class LocalDirectoryUpdater: public DirectoryUpdater
{
};

class RemoteDirectoryUpdater: public DirectoryUpdater
{
};
