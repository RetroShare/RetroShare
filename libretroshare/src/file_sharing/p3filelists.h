// This class is responsible for
//		- maintaining a list of shared file hierarchies for each known friends
//		- talking to the GUI
//			- providing handles for the directory tree listing GUI
//			- providing search handles for FT
//		- keeping these lists up to date
//		- sending our own file list to friends depending on the defined access rights
//		- serving file search requests from other services such as file transfer
//
//	p3FileList does the following micro-tasks:
//		- tick the watchers
//		- get incoming info from the service layer, which can be:
//			- directory content request => the directory content is shared to the friend
//			- directory content         => the directory watcher is notified
//		- keep two queues of update requests: 
//			- fast queue that is handled in highest priority. This one is used for e.g. updating while browsing.
//			- slow queue that is handled slowly. Used in background update of shared directories.
//
// The file lists are not directry updated. A FileListWatcher class is responsible for this
// in every case. 
//
class p3FileLists: public p3Service
{
	public:
		p3FileLists() ;


};

