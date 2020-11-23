/*******************************************************************************
 * libretroshare/src/file_sharing: file_sharing_defaults.h                     *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2016 by Mr.Alice <mralice@users.sourceforge.net>                  *
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

static const uint32_t DELAY_BETWEEN_DIRECTORY_UPDATES           =  600 ; // 10 minutes
static const uint32_t DELAY_BETWEEN_REMOTE_DIRECTORY_SYNC_REQ   =  120 ; // 2 minutes
static const uint32_t DELAY_BETWEEN_LOCAL_DIRECTORIES_TS_UPDATE =   20 ; // 20 sec. But we only update for real if something has changed.
static const uint32_t DELAY_BETWEEN_REMOTE_DIRECTORIES_SWEEP    =   60 ; // 60 sec.
static const uint32_t DELAY_BETWEEN_EXTRA_FILES_CACHE_UPDATES   =    2 ; //  2 sec.

static const uint32_t DELAY_BEFORE_DELETE_NON_EMPTY_REMOTE_DIR  = 60*24*86400 ; // delete non empty remoe directories after 60 days of inactivity
static const uint32_t DELAY_BEFORE_DELETE_EMPTY_REMOTE_DIR      =  5*24*86400 ; // delete empty remote directories after 5 days of inactivity

static const std::string HASH_CACHE_DURATION_SS                 = "HASH_CACHE_DURATION" ;	             // key string to store hash remembering time
static const std::string WATCH_FILE_DURATION_SS                 = "WATCH_FILES_DELAY" ;		             // key to store delay before re-checking for new files
static const std::string WATCH_FILE_ENABLED_SS                  = "WATCH_FILES_ENABLED"; 	             // key to store ON/OFF flags for file whatch
static const std::string TRUST_FRIEND_NODES_FOR_BANNED_FILES_SS = "TRUST_FRIEND_NODES_FOR_BANNED_FILES"; // should we trust friends for banned files or not
static const std::string FOLLOW_SYMLINKS_SS                     = "FOLLOW_SYMLINKS"; 	 	             // dereference symbolic links, or just ignore them.
static const std::string IGNORE_DUPLICATES                      = "IGNORE_DUPLICATES"; 	             	 // do not index files that are referenced multiple times because of links
static const std::string WATCH_HASH_SALT_SS                     = "WATCH_HASH_SALT"; 	 	             // Salt that is used to hash directory names
static const std::string IGNORED_PREFIXES_SS                    = "IGNORED_PREFIXES"; 	 	             // ignore file prefixes
static const std::string IGNORED_SUFFIXES_SS                    = "IGNORED_SUFFIXES"; 	 	             // ignore file suffixes
static const std::string IGNORE_LIST_FLAGS_SS                   = "IGNORED_FLAGS"; 	 	 	             // ignore file flags
static const std::string MAX_SHARE_DEPTH                        = "MAX_SHARE_DEPTH"; 	 	             // maximum depth of shared directories

static const std::string FILE_SHARING_DIR_NAME       = "file_sharing" ;			 // hard-coded directory name to store friend file lists, hash cache, etc.
static const std::string HASH_CACHE_FILE_NAME        = "hash_cache.bin" ;		 // hard-coded directory name to store encrypted hash cache.
static const std::string LOCAL_SHARED_DIRS_FILE_NAME = "local_dir_hierarchy.bin" ;	 // hard-coded directory name to store encrypted local dir hierarchy.

static const uint32_t MIN_INTERVAL_BETWEEN_HASH_CACHE_SAVE         = 20 ;    // never save hash cache more often than every 20 secs.
static const uint32_t MIN_INTERVAL_BETWEEN_REMOTE_DIRECTORY_SAVE   = 23 ;    // never save remote directories more often than this
static const uint32_t MIN_TIME_AFTER_LAST_MODIFICATION             = 10 ;    // never hash a file that is just being modified, otherwise we end up with a corrupted hash

static const uint32_t MAX_DIR_SYNC_RESPONSE_DATA_SIZE              = 20000 ; // Maximum RsItem data size in bytes for serialised directory transmission
static const uint32_t DEFAULT_HASH_STORAGE_DURATION_DAYS           = 30 ;    // remember deleted/inaccessible files for 30 days

static const uint32_t NB_FRIEND_INDEX_BITS_32BITS                    = 10 ;			// Do not change this!
static const uint32_t NB_ENTRY_INDEX_BITS_32BITS                     = 22 ;			// Do not change this!
static const uint32_t ENTRY_INDEX_BIT_MASK_32BITS                    = 0x003fffff ;	// used for storing (EntryIndex,Friend) couples into a 32bits pointer. Depends on the two values just before. Dont change!

static const uint64_t NB_FRIEND_INDEX_BITS_64BITS                    = 24 ;			// Do not change this!
static const uint64_t NB_ENTRY_INDEX_BITS_64BITS                     = 40 ;			// Do not change this!
static const uint64_t ENTRY_INDEX_BIT_MASK_64BITS                    = 0x0000000fffffffff ;	// used for storing (EntryIndex,Friend) couples into a 32bits pointer. Depends on the two values just before. Dont change!

static const uint32_t DELAY_BEFORE_DROP_REQUEST               = 600; 			// every 10 min

static const bool FOLLOW_SYMLINKS_DEFAULT                     = true;
static const bool TRUST_FRIEND_NODES_FOR_BANNED_FILES_DEFAULT = true;

static const uint32_t FL_BASE_TMP_SECTION_SIZE = 4096 ;
