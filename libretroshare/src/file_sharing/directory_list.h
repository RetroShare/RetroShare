// This class keeps a shared directory. It's quite the equivalent of the old "FileIndex" class
// The main difference is that it is 
// 	- extensible
// 	- fast to search (at least for hashes). Should provide possibly multiple search handles for 
// 		the same file, e.g. if connexion is encrypted.
// 	- abstracts the browsing in a same manner.
//
class SharedDirectoryList
{
	public:

		DirEntry mRoot ;
};
