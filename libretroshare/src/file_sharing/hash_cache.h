/*******************************************************************************
 * libretroshare/src/file_sharing: hash_cache.h                                *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2018 by Mr.Alice <mralice@users.sourceforge.net>                  *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Lesser General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Lesser General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Lesser General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 ******************************************************************************/


#pragma once

#include <map>
#include "util/rsthreads.h"
#include "retroshare/rsfiles.h"
#include "util/rstime.h"

/*!
 * \brief The HashStorageClient class
 * 		Used by clients of the hash cache for receiving hash results when done. This is asynchrone of course since hashing
 * 		might be quite costly.
 */
class HashStorageClient
{
public:
    HashStorageClient() {}
    virtual ~HashStorageClient() {}

    // the result of the hashing info is sent to this method

    virtual void hash_callback(uint32_t client_param, const std::string& name, const RsFileHash& hash, uint64_t size)=0;

    // this method is used to check that the client param is still valid just before hashing. This avoids hashing files
    // that are still in queue while removed from shared lists.

    virtual bool hash_confirm(uint32_t client_param)=0 ;
};

class HashStorage: public RsTickingThread
{
public:
    explicit HashStorage(const std::string& save_file_name) ;

    /*!
     * \brief requestHash  Requests the hash for the given file, assuming size and mod_time are the same.
     *
     * \param full_path    Full path to reach the file
     * \param size         Actual file size
     * \param mod_time     Actual file modification time
     * \param known_hash   Returned hash for the file.
     * \param c            Hash cache client to which the hash should be sent once calculated
     * \param client_param Param to be passed to the client callback. Useful if the client needs a file ID.
     *
     * \return true if the supplied hash info is up to date.
     */
    bool requestHash(const  std::string& full_path, uint64_t size, rstime_t mod_time, RsFileHash& known_hash, HashStorageClient *c, uint32_t client_param) ;

    struct HashStorageInfo
    {
        std::string filename ;		// full path of the file
        uint64_t size ;
        uint32_t time_stamp ;		// last time the hash was tested/requested
        uint32_t modf_stamp ;
        RsFileHash hash ;
    } ;

    // interaction with GUI, called from p3FileLists
    void setRememberHashFilesDuration(uint32_t days) { mMaxStorageDurationDays = days ; }		// duration for which the hash is kept even if the file is not shared anymore
    uint32_t rememberHashFilesDuration() const { return mMaxStorageDurationDays ; }
    void clear() { mFiles.clear(); mChanged=true; }												// drop all known hashes. Not something to do, except if you want to rehash the entire database
    bool empty() const { return mFiles.empty() ; }
	void togglePauseHashingProcess() ;
	bool hashingProcessPaused();

    // Functions called by the thread

    virtual void data_tick() ;

    friend std::ostream& operator<<(std::ostream& o,const HashStorageInfo& info) ;
private:
    /*!
     * \brief clean
     * 		This function is responsible for removing old hashes, etc
     */
    void clean() ;

    // loading/saving the entire hash database to a file

    void locked_save() ;
    bool locked_load() ;
    bool try_load_import_old_hash_cache();

    bool readHashStorageInfo(const unsigned char *data,uint32_t total_size,uint32_t& offset,HashStorageInfo& info) const;
    bool writeHashStorageInfo(unsigned char *& data,uint32_t&  total_size,uint32_t& offset,const HashStorageInfo& info) const;

    // Local configuration and storage

    uint32_t mMaxStorageDurationDays ; 				// maximum duration of un-requested cache entries
    std::map<std::string, HashStorageInfo> mFiles ;	// stored as (full_path, hash_info)
    std::string mFilePath ;							// file where the hash database is stored
    bool mChanged ;
	bool mHashingProcessPaused ;

    struct FileHashJob
    {
        std::string full_path;		// canonicalized file name (means: symlinks removed, loops removed, etc)
        std::string real_path;		// path supplied by the client.
        uint64_t size ;
        HashStorageClient *client;
        uint32_t client_param ;
        rstime_t ts;
    };

    // current work

    std::map<std::string,FileHashJob> mFilesToHash ;

    // thread/mutex stuff

    RsMutex mHashMtx ;
    bool mRunning;
    uint64_t mHashCounter;
    uint32_t mInactivitySleepTime ;
    uint64_t mTotalSizeToHash ;
    uint64_t mTotalHashedSize ;
    uint64_t mTotalFilesToHash ;
    rstime_t mLastSaveTime ;

	// The following is used to estimate hashing speed.

	double mHashingTime ;
	uint64_t mHashedBytes ;
	uint32_t mCurrentHashingSpeed ; // in MB/s
};

