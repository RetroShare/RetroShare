#include "directory_storage.h"
#include "directory_updater.h"

void RemoteDirectoryUpdater::tick()
{
	// use the stored iterator
}

void LocalDirectoryUpdater::tick()
{
#ifdef TODO
	// recursive update algorithm works that way:
	// 	- the external loop starts on the shared directory list and goes through sub-directories
	// 	- at the same time, it updates the list of shared directories
	// 	- the information that is costly to compute (the hash) is store externally into a separate structure.
	// 	- doing so, changing directory names or moving files between directories does not cause a re-hash of the content. 
	//
	for(LocalSharedDirectoryMap::iterator it(mLocalSharedDirs);it;++it)
	{
		librs::util::FolderIterator dirIt(realpath);
		if(!dirIt.isValid())
			mLocalSharedDirs.removeDirectory(it) ;	// this is a complex operation since it needs to *update* it so that it is kept consistent.

		while(dirIt.readdir())
		{

		}

		if(mHashCache.getHash(realpath+"/"+fe.name,fe.size,fe.modtime,fe.hash))
		{
			;
		}
	}
#endif
}
