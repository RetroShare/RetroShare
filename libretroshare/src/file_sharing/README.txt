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

Classes
-------
   Rs

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
      - time_t        Last modification time

   LocalFileInfo: public FileInfo
      - time_t        Last data access time
      - uint64_t      Total data uploaded
      - uint32_t      ShareFlags
   
   SharedDirectory
      - parent groups
      - group flags

Syncing between peers
---------------------

Generating sync events
   * Client side
      - for each directory, in breadth first order
         - if directory has changed, or last update is old
            => push a sync request

   * Server side
      - after a change, broadcast a "directory changed" packet to all connected friends
   

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

- complete this file until a proper description of the whole thing is achieved.
- create a new directory and implement the .h for the basic functionality
- look into existing code in ftServer for the integration, but don't change anything yet
- setup class hierarchy
- merge hash cache into file lists.

- optionally
   - change the saving system of FileIndex to make it locally encrypted and compact

TODO
====
   [ ] implement directory updater
         [ ] local directory updater
         [ ] remote directory updater

   [ ] implement directory handler
   [ ] implement p3FileLists with minimal functonality: no exchange. Only storage of own



















