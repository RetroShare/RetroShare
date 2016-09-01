#pragma once

static const uint32_t DELAY_BETWEEN_DIRECTORY_UPDATES           = 100 ; // 10 seconds for testing. Should be much more!!
static const uint32_t DELAY_BETWEEN_REMOTE_DIRECTORY_SYNC_REQ   = 10 ; // 10 seconds for testing. Should be much more!!
static const uint32_t DELAY_BETWEEN_LOCAL_DIRECTORIES_TS_UPDATE = 10 ; // 10 seconds for testing. Should be much more!!

static const std::string HASH_CACHE_DURATION_SS = "HASH_CACHE_DURATION" ;	// key string to store hash remembering time
static const std::string WATCH_FILE_DURATION_SS = "WATCH_FILES_DELAY" ;		// key to store delay before re-checking for new files

