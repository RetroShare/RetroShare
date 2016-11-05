/*
 * RetroShare C++ File sharing default variables
 *
 *      file_sharing/file_sharing_defaults.h
 *
 * Copyright 2016 by Mr.Alice
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 *
 * Please report all bugs and problems to "retroshare.project@gmail.com".
 *
 */


#pragma once

static const uint32_t DELAY_BETWEEN_DIRECTORY_UPDATES           = 600 ; // 10 minutes
static const uint32_t DELAY_BETWEEN_REMOTE_DIRECTORY_SYNC_REQ   = 120 ; // 2 minutes
static const uint32_t DELAY_BETWEEN_LOCAL_DIRECTORIES_TS_UPDATE =  20 ; // 20 sec. But we only update for real if something has changed.
static const uint32_t DELAY_BETWEEN_REMOTE_DIRECTORIES_SWEEP    =  60 ; // 60 sec.

static const std::string HASH_CACHE_DURATION_SS = "HASH_CACHE_DURATION" ;	 // key string to store hash remembering time
static const std::string WATCH_FILE_DURATION_SS = "WATCH_FILES_DELAY" ;		 // key to store delay before re-checking for new files
static const std::string WATCH_FILE_ENABLED_SS  = "WATCH_FILES_ENABLED"; 	 // key to store ON/OFF flags for file whatch
static const std::string WATCH_HASH_SALT_SS     = "WATCH_HASH_SALT"; 	 	 // Salt that is used to hash directory names

static const std::string FILE_SHARING_DIR_NAME  = "file_sharing" ;			 // hard-coded directory name to store friend file lists, hash cache, etc.
static const std::string HASH_CACHE_FILE_NAME   = "hash_cache.bin" ;		 // hard-coded directory name to store encrypted hash cache.

static const uint32_t MIN_INTERVAL_BETWEEN_HASH_CACHE_SAVE         = 20 ;    // never save hash cache more often than every 20 secs.
static const uint32_t MIN_INTERVAL_BETWEEN_REMOTE_DIRECTORY_SAVE   = 23 ;    // never save remote directories more often than this

static const uint32_t MAX_DIR_SYNC_RESPONSE_DATA_SIZE              = 20000 ; // Maximum RsItem data size in bytes for serialised directory transmission
static const uint32_t DEFAULT_HASH_STORAGE_DURATION_DAYS           = 30 ;    // remember deleted/inaccessible files for 30 days

static const uint32_t NB_FRIEND_INDEX_BITS                    = 10 ;			// Do not change this!
static const uint32_t NB_ENTRY_INDEX_BITS                     = 22 ;			// Do not change this!
static const uint32_t ENTRY_INDEX_BIT_MASK                    = 0x003fffff ;	// used for storing (EntryIndex,Friend) couples into a 32bits pointer. Depends on the two values just before. Dont change!
static const uint32_t DELAY_BEFORE_DROP_REQUEST               = 600; 			// every 10 min
