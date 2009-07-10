#ifndef RSEFS_H
#define RSEFS_H


/*
 *  From rsnotify.h
 */

const uint32_t RS_SYS_ERROR 	= 0x0001;
const uint32_t RS_SYS_WARNING 	= 0x0002;
const uint32_t RS_SYS_INFO 	= 0x0004;

const uint32_t RS_POPUP_MSG	= 0x0001;
const uint32_t RS_POPUP_CHAT	= 0x0002;
const uint32_t RS_POPUP_CALL	= 0x0004;
const uint32_t RS_POPUP_CONNECT = 0x0008;

/* CHAT flags are here - so they are in the same place as
 * other Notify flags... not used by libretroshare though
 */
const uint32_t RS_CHAT_OPEN_NEW		= 0x0001;
const uint32_t RS_CHAT_REOPEN		= 0x0002;
const uint32_t RS_CHAT_FOCUS		= 0x0004;

const uint32_t RS_FEED_TYPE_PEER 	= 0x0010;
const uint32_t RS_FEED_TYPE_CHAN 	= 0x0020;
const uint32_t RS_FEED_TYPE_FORUM 	= 0x0040;
const uint32_t RS_FEED_TYPE_BLOG 	= 0x0080;
const uint32_t RS_FEED_TYPE_CHAT 	= 0x0100;
const uint32_t RS_FEED_TYPE_MSG 	= 0x0200;
const uint32_t RS_FEED_TYPE_FILES 	= 0x0400;

const uint32_t RS_FEED_ITEM_PEER_CONNECT	 = RS_FEED_TYPE_PEER  | 0x0001;
const uint32_t RS_FEED_ITEM_PEER_DISCONNECT	 = RS_FEED_TYPE_PEER  | 0x0002;
const uint32_t RS_FEED_ITEM_PEER_NEW		 = RS_FEED_TYPE_PEER  | 0x0003;
const uint32_t RS_FEED_ITEM_PEER_HELLO		 = RS_FEED_TYPE_PEER  | 0x0004;

const uint32_t RS_FEED_ITEM_CHAN_NEW		 = RS_FEED_TYPE_CHAN  | 0x0001;
const uint32_t RS_FEED_ITEM_CHAN_UPDATE		 = RS_FEED_TYPE_CHAN  | 0x0002;
const uint32_t RS_FEED_ITEM_CHAN_MSG		 = RS_FEED_TYPE_CHAN  | 0x0003;

const uint32_t RS_FEED_ITEM_FORUM_NEW		 = RS_FEED_TYPE_FORUM | 0x0001;
const uint32_t RS_FEED_ITEM_FORUM_UPDATE	 = RS_FEED_TYPE_FORUM | 0x0002;
const uint32_t RS_FEED_ITEM_FORUM_MSG		 = RS_FEED_TYPE_FORUM | 0x0003;

const uint32_t RS_FEED_ITEM_BLOG_MSG		 = RS_FEED_TYPE_BLOG  | 0x0001;
const uint32_t RS_FEED_ITEM_CHAT_NEW		 = RS_FEED_TYPE_CHAT  | 0x0001;
const uint32_t RS_FEED_ITEM_MESSAGE		 = RS_FEED_TYPE_MSG   | 0x0001;
const uint32_t RS_FEED_ITEM_FILES_NEW		 = RS_FEED_TYPE_FILES | 0x0001;

/*
 *  From rsnotifybase.h
 */

const int NOTIFY_LIST_NEIGHBOURS   = 1;
const int NOTIFY_LIST_FRIENDS      = 2;
const int NOTIFY_LIST_DIRLIST      = 3;
const int NOTIFY_LIST_SEARCHLIST   = 4;
const int NOTIFY_LIST_MESSAGELIST  = 5;
const int NOTIFY_LIST_CHANNELLIST  = 6;
const int NOTIFY_LIST_TRANSFERLIST = 7;
const int NOTIFY_LIST_CONFIG       = 8;

const int NOTIFY_TYPE_SAME   = 0x01;
const int NOTIFY_TYPE_MOD    = 0x02; /* general purpose, check all */
const int NOTIFY_TYPE_ADD    = 0x04; /* flagged additions */
const int NOTIFY_TYPE_DEL    = 0x08; /* flagged deletions */


/*
 *  From rsfiles.h
 */

/* These are used mainly by ftController at the moment */
const uint32_t RS_FILE_CTRL_PAUSE	 = 0x00000100;
const uint32_t RS_FILE_CTRL_START	 = 0x00000200;

const uint32_t RS_FILE_RATE_TRICKLE	 = 0x00000001;
const uint32_t RS_FILE_RATE_SLOW	 = 0x00000002;
const uint32_t RS_FILE_RATE_STANDARD	 = 0x00000003;
const uint32_t RS_FILE_RATE_FAST	 = 0x00000004;
const uint32_t RS_FILE_RATE_STREAM_AUDIO = 0x00000005;
const uint32_t RS_FILE_RATE_STREAM_VIDEO = 0x00000006;

const uint32_t RS_FILE_PEER_ONLINE 	 = 0x00001000;
const uint32_t RS_FILE_PEER_OFFLINE 	 = 0x00002000;

/************************************
 * Used To indicate where to search.
 *
 * The Order of these is very important,
 * it specifies the search order too.
 *
 */

const uint32_t RS_FILE_HINTS_MASK	 = 0x00ffffff;

const uint32_t RS_FILE_HINTS_CACHE	 = 0x00000001;
const uint32_t RS_FILE_HINTS_EXTRA	 = 0x00000002;
const uint32_t RS_FILE_HINTS_LOCAL	 = 0x00000004;
const uint32_t RS_FILE_HINTS_REMOTE	 = 0x00000008;
const uint32_t RS_FILE_HINTS_DOWNLOAD= 0x00000010;
const uint32_t RS_FILE_HINTS_UPLOAD	 = 0x00000020;
const uint32_t RS_FILE_HINTS_TURTLE	 = 0x00000040;


const uint32_t RS_FILE_HINTS_SPEC_ONLY	 = 0x01000000;
const uint32_t RS_FILE_HINTS_NO_SEARCH   = 0x02000000;

/* Callback Codes */
//const uint32_t RS_FILE_HINTS_CACHE	 = 0x00000001; // ALREADY EXISTS
const uint32_t RS_FILE_HINTS_MEDIA	 = 0x00001000;

const uint32_t RS_FILE_HINTS_BACKGROUND	 = 0x00002000; // To download slowly.

const uint32_t RS_FILE_EXTRA_DELETE	 = 0x0010;

const uint32_t CB_CODE_CACHE = 0x0001;
const uint32_t CB_CODE_EXTRA = 0x0002;
const uint32_t CB_CODE_MEDIA = 0x0004;


















#endif // RSEFS_H
