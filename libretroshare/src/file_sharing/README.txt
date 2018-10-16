Design document for the new file list sharing system.
====================================================

Big picture
-----------

   p3SharesManager
      Functionalities
         * contains the list of shared files
            - responds to search, requests for FileInfo
         * responsible for sync-ing file lists between peers

            Level 3: interface with GUI
               - regularly issue syncing requests in background
               - automatically issue syncing requests as files are browsed

            Level 2: internal stuff
               - examine syncing requests. Only get data

            Level 1: syncing system
               - list of pending requests with priorities, sort them and handle response from friends

            Level 0: interface with friends
               - serialisation/deserialisation of file list items

      FileIndex should support:
         - O(log(n)) search for hash
         - O(n)      search for regular exp
         - O(1)      remove of file and directory

      Members
         - list of file indexes
            => map of <peer id, FileIndex>
         - 

      Parent classes
         - p3Service (sends and receives information about shared files)

      Notes:
         - the same file should be able to be held by two different directories. Hash search will return a single hit.
         - the previously existing PersonEntry had no field and was overloading DirEntry, with overwritten file names, hashes etc. Super bad!

Directory storage file format
-----------------------------
   * should be extensible (xml or binary format? Binary, because it's going to be encrypted anyway)
      => works with binary fields
      => 

      [= version =]
      [= peer id =]
      [= num entries =]
      [= some information =]

      [entry tag] [entry size] [Field ID 01] [field size v 01] [Field data 01]  [Field ID 02] [field size v 02] [Field data 02] ...
      [entry tag] [entry size] [Field ID 01] [field size v 01] [Field data 01]  [Field ID 02] [field size v 02] [Field data 02] ...
      [entry tag] [entry size] [Field ID 01] [field size v 01] [Field data 01]  [Field ID 02] [field size v 02] [Field data 02] ...
         ...
                                     2             1-5                v               2             1-5               v
   * entry content
         Tag           | Content                              |  Size
       ----------------+--------------------------------------+------
         01            | sha1 hash                            |  20
         01            | sha1^2 hash                          |  20
         02            | file name                            |  < 512
         03            | file size                            |  8
         04            | dir name                             |  < 512
         05            | last modif time local                |  4
         06            | last modif time including sub-dirs   |  4

Classes
-------

   p3ShareManager
      - tick()
         * handle pending syncing requests => sends items
         * watch shared directories
            => hash new files
            => updates shared files
         * 

      - list of shared parent directories, with share flags

   FileIndex                                    // should support hierarchical navigation, load/save
      -  

   LocalFileIndex: public FileIndex
      - std::map<RsFileHash, LocalFileInfo>     // used for search

   FileInfo
      - std::string   name
      - RsFileHash    hash
      - uint64_t      size
      - rstime_t        Last modification time

   LocalFileInfo: public FileInfo
      - rstime_t        Last data access time
      - uint64_t      Total data uploaded
      - uint32_t      ShareFlags
   
   SharedDirectory
      - parent groups
      - group flags

Best data structure for file index
----------------------------------

                      | Hash map          map          list
      ----------------+-----------------+------------+--------------
      Adding          | Constant        | log(n)     | O(n)             
      Hash search     | Constant        | log(n)     | O(n)             
      Name/exp search | O(n)            | O(n)       | O(n)
      Recursive browse| Constant        | log(n)     | O(n)

      Should we use the same struct for files and directories?

      Sol 1:
         DirClass + PersonClass + FileEntry class
            - each has pointers to elements list of the same type
            - lists are handled for Files (all file entries), 

            Directories are represented by the hash of the full path 

      Sol 2:
         Same class for all elements, in a single hash map. Each element is
         defined by its type (Dir, Person, File) which all have a hash.

Syncing between peers
---------------------

Generating sync events
   * Client side
      - for each directory, in breadth first order
         - if directory has changed, or last update is old
            => push a sync request
         - store the peer's last up time. Compare with peer uptimes recursively.

   * Server side
      - after a change, broadcast a "directory changed" packet to all connected friends

   * directoy updater
      - crawl through directories
         - compare TS of files, missing files, new files
         - feed a queue of files to hash
      - directory whatcher gets notified when files are hashed

      - a separate component hashes files (FileHashingProcess)

   DirectoryWatcher (watches a hierarchy)        File List (stores a directory hierarchy)
         |                                          |
         |                                          |
         |                                          |
         +-----------------------+------------------+
                                 |                  |
                        Shared File Service         |
                                 |                  |
                                 |                  |
                                 +----------- own file list -------+---------- Encrypted/compressed save to disk
                                 |                  |              |
                                 +----------- friend file lists ---+
Roadmap
-------

[X] complete this file until a proper description of the whole thing is achieved.
[X] create a new directory and implement the .h for the basic functionality
[ ] look into existing code in ftServer for the integration, but don't change anything yet
[X] setup class hierarchy
[ ] merge hash cache into file lists.
[ ] new format for saving of FileIndex to make it locally encrypted, compact and extensible
[ ] create basic directory functionality with own files: re-hash, and store
[ ] display own files in GUI, with proper update and working sort

TODO
====

   [ ] directory handler
         [ ] abstract functions to keep a directory and get updates to it.
         [ ] hierarchical storage representation.
               [ ] allow add/delete entries
               [ ] auto-cleanup

   [ ] directory updater
         [ ] abstract layer
               [ ] crawls the directory and ask updates

         [ ] derive local directory updater
               [ ] crawl local files, and asks updates to storage class
         [ ] derive remote directory updater
               [ ] crawl stored files, and request updates to storage class

   [ ] load/save of directory content. Should be extensible
   [ ] p3FileLists with minimal functonality: no exchange. Only storage of own file lists
   [ ] service (items) for p3FileLists
   [ ] connect RemoteDirModel to new system
   [ ] test GUI functions
   [ ] test update between peers




















