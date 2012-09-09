/*
 * RetroShare External Interface.
 *
 * Copyright 2012-2012 by Robert Fernie.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2.1 as published by the Free Software Foundation.
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
 * Please report all bugs and problems to "retroshare@lunamutt.com".
 *
 */

#include <retroshare/rspeers.h>
#include <retroshare/rsfiles.h>
#include <retroshare/rsturtle.h>
#include "util/rsstring.h"
#include "util/rsdir.h"

#include "menu/menus.h"

// Can't use: Q, H, T, U (universal)
// or L, N, P (list ops).
// or 0-9,a-f (list selection).

#define 	MENU_FRIENDS_KEY_ADD  			'a'
#define 	MENU_FRIENDS_KEY_VIEW  			'v'
#define 	MENU_FRIENDS_KEY_REMOVE  		'd'
#define 	MENU_FRIENDS_KEY_CHAT  			'c'

#define 	MENU_TRANSFER_KEY_STOP  		's'
#define 	MENU_TRANSFER_KEY_CANCEL  		'c'

#define 	MENU_SEARCH_KEY_ADD  			'a'
#define 	MENU_SEARCH_KEY_REMOVE  		'd'
#define 	MENU_SEARCH_KEY_VIEW  			'v'
#define 	MENU_SEARCH_KEY_DOWNLOAD  		'g'

#define 	MENU_FORUMS_KEY_ADD  			'a'
#define 	MENU_FORUMS_KEY_VIEW  			'v'
#define 	MENU_FORUMS_KEY_REMOVE  		'd'
#define 	MENU_FORUMS_KEY_CHAT  			'c'

#define 	MENU_SHARED_KEY_EXPAND 			'e'
#define 	MENU_SHARED_KEY_UNSHARE			's'
#define 	MENU_SHARED_KEY_ADD			'a'
#define 	MENU_SHARED_KEY_PUBLIC			'c'
#define 	MENU_SHARED_KEY_BROWSABLE		'b'

#define 	MENU_TOPLEVEL_KEY_FRIENDS  		'f'
#define 	MENU_TOPLEVEL_KEY_NETWORK  		'w'
#define 	MENU_TOPLEVEL_KEY_TRANSFER  		'd'
#define 	MENU_TOPLEVEL_KEY_SEARCH  		's'
#define 	MENU_TOPLEVEL_KEY_FORUMS  		'o'
#define 	MENU_TOPLEVEL_KEY_SHARED  		'k'  //If you know of a key which fits better, just change it ;-)
#define 	MENU_TOPLEVEL_KEY_UPLOADS 		'e'


Menu *CreateMenuStructure(NotifyTxt *notify)
{
	/* construct Friends Menu */
	MenuList *friends = new MenuListFriends();
	// No Friends Operations Completed Yet!
	//friends->addMenuItem(MENU_FRIENDS_KEY_ADD, new MenuOpFriendsAdd());
	//friends->addMenuItem(MENU_FRIENDS_KEY_VIEW, new MenuOpFriendsViewDetail());
	//friends->addMenuItem(MENU_FRIENDS_KEY_REMOVE, new MenuOpFriendsRemove());
	//friends->addMenuItem(MENU_FRIENDS_KEY_CHAT, new MenuOpFriendsChat());

	MenuList *network = new MenuListNetwork();

	MenuList *transfers = new MenuListTransfer();
	//transfers->addMenuItem(MENU_TRANSFER_KEY_STOP, new MenuOpTransferStop());
	transfers->addMenuItem(MENU_TRANSFER_KEY_CANCEL, new MenuOpTransferCancel());

	MenuList *search = new MenuListSearch(notify);
	MenuList *searchlist = new MenuListSearchList(notify);
	search->addMenuItem(MENU_SEARCH_KEY_ADD, new MenuOpSearchNew(notify));
	//search->addMenuItem(MENU_SEARCH_KEY_REMOVE, new MenuOpSearchDelete());
	search->addMenuItem(MENU_SEARCH_KEY_VIEW, searchlist);
	searchlist->addMenuItem(MENU_SEARCH_KEY_DOWNLOAD, new MenuOpSearchListDownload());

	// Forums - TODO.
	//MenuList *forums = new MenuListForums();
	//forums->addMenuItem(MENU_FRIENDS_KEY_ADD, new MenuOpFriendsAdd());
	//forums->addMenuItem(MENU_FRIENDS_KEY_VIEW, new MenuOpFriendsViewDetail());
	//forums->addMenuItem(MENU_FRIENDS_KEY_REMOVE, new MenuOpFriendsRemove());
	//forums->addMenuItem(MENU_FRIENDS_KEY_CHAT, new MenuOpFriendsChat());

	// Shared folders Menu
	MenuList *shared = new MenuListShared();
	shared->addMenuItem(MENU_SHARED_KEY_ADD	, new MenuListSharedAddShare());
	shared->addMenuItem(MENU_SHARED_KEY_UNSHARE	, new MenuListSharedUnshare());
	shared->addMenuItem(MENU_SHARED_KEY_PUBLIC	, new MenuListSharedTogglePublic());
	shared->addMenuItem(MENU_SHARED_KEY_BROWSABLE , new MenuListSharedToggleBrowsable());

	/* Top Level Menu */
	Menu *tlm = new Menu("Top Level Menu");

	tlm->addMenuItem(MENU_TOPLEVEL_KEY_FRIENDS, friends);
	tlm->addMenuItem(MENU_TOPLEVEL_KEY_NETWORK, network);
	tlm->addMenuItem(MENU_TOPLEVEL_KEY_TRANSFER, transfers);
	tlm->addMenuItem(MENU_TOPLEVEL_KEY_SEARCH, search);
	//tlm->addMenuItem(MENU_TOPLEVEL_KEY_FORUMS, forums);
	tlm->addMenuItem(MENU_TOPLEVEL_KEY_SHARED, shared);

	return tlm;
}


/************
 * Friends Menu
 */


uint32_t MenuListFriends::op()
{
	MenuList::op();

	rsPeers->getGPGAcceptedList(mList);
	//rsPeers->getGPGValidList(mList);
		
	return MENU_OP_SUBMENU;
}

int MenuListFriends::getEntryDesc(int idx, std::string &desc)
{
	std::string gpgId;

	if (!getListEntry(idx, gpgId))
	{
		/* error */
		return 0;
	}

	RsPeerDetails details;
	if (rsPeers->getPeerDetails(gpgId, details))
	{
		if (details.accept_connection) 
		{
			desc = "Friend: ";
		}
		else
		{
			desc = "Neighbour: ";
		}
	
		desc += "<" + gpgId + ">  ";
		desc += details.name;
	}
	return 1;
}


uint32_t MenuListNetwork::op()
{
	MenuList::op();

	rsPeers->getGPGValidList(mList);
	//rsPeers->getGPGAcceptedList(mList);
		
	return MENU_OP_SUBMENU;
}


uint32_t MenuOpFriendsAdd::op_basic(std::string key)
{


	return MENU_OP_INSTANT;
}

uint32_t MenuOpFriendsViewDetail::op_basic(std::string key)
{


	return MENU_OP_INSTANT;
}


uint32_t MenuOpFriendsRemove::op_basic(std::string key)
{


	return MENU_OP_INSTANT;
}


uint32_t MenuOpFriendsChat::op_basic(std::string key)
{


	return MENU_OP_INSTANT;
}


/************
 * Transfer Menu
 */


uint32_t MenuListTransfer::op()
{
	MenuList::op();

	/* load friend list*/
	rsFiles->FileDownloads(mList);
	
	return MENU_OP_SUBMENU;
}

int MenuListTransfer::getEntryDesc(int idx, std::string &desc)
{
	std::string hash;
	if (!getListEntry(idx, hash))
	{
		std::cout << "MenuListTransfer::getEntryDesc() No ListEntry";
		std::cout << std::endl;
		return 0;
	}

	FileInfo info;
	if (!rsFiles->FileDetails(hash, RS_FILE_HINTS_DOWNLOAD, info))
	{
		std::cout << "MenuListTransfer::getEntryDesc() No FileDetails";
		std::cout << std::endl;
		return 0;
	}

	float frac = 100.0 * (float) info.transfered / info.size;

	if (frac != 1.0)
	{
		if (info.tfRate > 0)
		{
			rs_sprintf(desc, "<%s> Downloading %3.2f%% from %d Peers @%3.1fKB/s. Name: %s", 
				info.hash.c_str(), frac, info.peers.size(), info.tfRate, info.fname.c_str());
		}
		else
		{
			rs_sprintf(desc, "<%s> Stalled %3.2f%% Name: %s", 
				info.hash.c_str(), frac, info.fname.c_str());
		}
	}
	else
	{
		rs_sprintf(desc, "<%s> Done! Name: %s", info.hash.c_str(), info.fname.c_str());
	}
	return MENU_OP_INSTANT;
}

uint32_t MenuOpTransferStop::op_basic(std::string key)
{
	rsFiles->FileCancel(key);
	parent()->setOpMessage("Stopped Transfer");
	return MENU_OP_INSTANT;
}

uint32_t MenuOpTransferCancel::op_basic(std::string key)
{
	rsFiles->FileCancel(key);
	parent()->setOpMessage("Stopped Transfer");
	return MENU_OP_INSTANT;
}


/************
 * Search Menu
 */



uint32_t MenuListSearch::op()
{
	mSelectIdx = -1;
	mCursor = 0;

	/* Don't reset List -> done dynamically */

	return MENU_OP_SUBMENU;
}

int MenuListSearch::getEntryDesc(int idx, std::string &desc)
{
	std::string strSearchId;

	if (!getListEntry(idx, strSearchId))
	{
		/* error */
		return 0;
	}

	std::map<std::string, std::string>::iterator it;
	std::map<std::string, uint32_t>::iterator sit;
	it = mSearchTerms.find(strSearchId);
	sit = mSearchIds.find(strSearchId);
	if ((it == mSearchTerms.end()) || (sit == mSearchIds.end()))
	{
		/* error */
		return 0;
	}

	rs_sprintf(desc, "Search(\"%s\") Found %d matches so far", 
		it->second.c_str(), mNotify->getSearchResultCount(sit->second));
	return 1;
}


int MenuListSearch::getCurrentSearchId(uint32_t &id)
{
	std::string strSearchId;

	if (!getListEntry(mSelectIdx, strSearchId))
	{
		/* error */
		return 0;
	}

	std::map<std::string, uint32_t>::iterator sit;
	sit = mSearchIds.find(strSearchId);
	if (sit == mSearchIds.end())
	{
		/* error */
		return 0;
	}

	id = sit->second;
	return 1;
}

int MenuListSearch::storeSearch(uint32_t searchId, std::string match_string)
{
	std::cout << "MenuListSearch::storeSearch(" << searchId << " => ";
	std::cout << match_string;
	std::cout << std::endl;

	std::string strSearchId;
	rs_sprintf(strSearchId, "%lu", searchId);
	mList.push_back(strSearchId);

	mSearchTerms[strSearchId] = match_string;
	mSearchIds[strSearchId] = searchId;

	return 1;
}

int MenuListSearch::removeSearch(std::string strSearchId)
{
	std::cout << "MenuListSearch::removeSearch(" << strSearchId << ")";
	std::cout << std::endl;

	std::map<std::string, uint32_t>::iterator it;
	it = mSearchIds.find(strSearchId);
	if (it != mSearchIds.end())
	{

		/* cancel search */
		// CAN'T DO!!!

		/* clear results from Notify Collector */
		mNotify->clearSearchId(it->second);

		/* cleanup local maps */
		mSearchIds.erase(it);

		/* cleanup terms maps (TODO) */
	}

	return 1;
}

uint32_t MenuOpSearchNew::drawPage(uint32_t drawFlags, std::string &buffer)
{
        buffer += "Enter New Search Term > ";
	return 1;
}



uint32_t MenuOpSearchNew::process_lines(std::string input)
{
	/* launch search */
	if (input.size() < 4)
	{
		std::cout << "MenuOpSearchNew::process_lines() ERROR Input too small";
		std::cout << std::endl;
		return MENU_PROCESS_ERROR;
	}

	std::string search = input.substr(0, input.size() - 1); // remove \n.
	uint32_t searchId = (uint32_t) rsTurtle->turtleSearch(search);
	mNotify->collectSearchResults(searchId);

	/* store request in parent */
	MenuListSearch *ms = dynamic_cast<MenuListSearch *>(parent());
	if (ms)
	{
		ms->storeSearch(searchId, search);
	}

	return MENU_PROCESS_DONE;
}

uint32_t MenuOpSearchDelete::op_basic(std::string key)
{

	return MENU_OP_INSTANT;
}


uint32_t MenuListSearchList::op()
{
	MenuList::op();
	return refresh();
}

uint32_t MenuListSearchList::refresh()
{
	Menu* p = parent();
	MenuListSearch *mls = dynamic_cast<MenuListSearch *>(p);
	if (!mls)
	{
		std::cout << "MenuListSearchList::refresh() mls not there";
		std::cout << std::endl;
		return MENU_OP_ERROR;
	}

	/* load friend list*/
	mList.clear();

	uint32_t searchId;
	if (!mls->getCurrentSearchId(searchId))
	{
		std::cout << "MenuListSearchList::refresh() currentIdx invalid";
		std::cout << std::endl;
		return MENU_OP_ERROR;
	}

	std::cout << "MenuListSearchList::refresh() searchId: " << searchId;
	std::cout << std::endl;

	std::list<TurtleFileInfo>::iterator it;
	mNotify->getSearchResults(searchId, mSearchResults);

	/* convert into useful list */
	for(it = mSearchResults.begin(); it != mSearchResults.end(); it++)
	{
		mList.push_back(it->hash);
	}

	mSelectIdx = -1;
	mCursor = 0;
	return MENU_OP_SUBMENU;
}

int MenuListSearchList::getEntryDesc(int idx, std::string &desc)
{
	std::list<TurtleFileInfo>::iterator it;
        int i = 0;
        for (it = mSearchResults.begin(); 
		(i < idx) && (it != mSearchResults.end()); it++, i++) ;

        if (it != mSearchResults.end())
        {
		rs_sprintf(desc, "<%s> Size: %llu Name:%s", 
			it->hash.c_str(), it->size, it->name.c_str());
                return MENU_ENTRY_OKAY;
        }
        return MENU_ENTRY_NONE;
}


int MenuListSearchList::downloadSelected()
{
	if (mSelectIdx < 0)
	{
		std::cout << "MenuListSearchList::downloadSelected() Invalid Selection";
		std::cout << std::endl;
        	return MENU_ENTRY_NONE;
	}

	std::list<TurtleFileInfo>::iterator it;
        int i = 0;
        for (it = mSearchResults.begin(); 
		(i < mSelectIdx) && (it != mSearchResults.end()); it++, i++) ;

        if (it != mSearchResults.end())
        {
		std::list<std::string> srcIds;
		if (rsFiles -> FileRequest(it->name, it->hash, it->size, 
				"", RS_FILE_HINTS_NETWORK_WIDE, srcIds))
		{
			std::cout << "MenuListSearchList::downloadSelected() Download Started";
			std::cout << std::endl;
		}
		else
		{
			std::cout << "MenuListSearchList::downloadSelected() Error Starting Download";
			std::cout << std::endl;
		}
                return MENU_ENTRY_OKAY;
        }
        return MENU_ENTRY_NONE;
}



uint32_t MenuOpSearchListDownload::op_basic(std::string key)
{
	Menu* p = parent();
	MenuListSearchList *mlsl = dynamic_cast<MenuListSearchList *>(p);
	if (!mlsl)
	{
		std::cout << "MenuOpSearchListDownload::op_basic() ERROR";
		std::cout << std::endl;
		return MENU_OP_ERROR;
	}

	mlsl->downloadSelected();

	return MENU_OP_INSTANT;
}

/************
 * Forums Menu
 */



uint32_t MenuListForums::op()
{
	/* load friend list*/
	mList.clear();
	rsPeers->getGPGAcceptedList(mList);
	mSelectIdx = 0;
		
	return MENU_OP_SUBMENU;
}


int MenuListForums::getEntryDesc(int idx, std::string &desc)
{


	return MENU_OP_INSTANT;
}

uint32_t MenuOpForumDetails::op_basic(std::string key)
{


	return MENU_OP_INSTANT;
}


uint32_t MenuOpForumSubscribe::op_basic(std::string key)
{


	return MENU_OP_INSTANT;
}


uint32_t MenuOpForumUnsubscribe::op_basic(std::string key)
{


	return MENU_OP_INSTANT;
}


uint32_t MenuOpForumCreate::op_basic(std::string key)
{


	return MENU_OP_INSTANT;
}


uint32_t MenuListForumMsgs::op()
{
	/* load friend list*/
	mList.clear();
	rsPeers->getGPGAcceptedList(mList);
	mSelectIdx = 0;
		
	return MENU_OP_SUBMENU;
}

int MenuListForumMsgs::getEntryDesc(int idx, std::string &desc)
{


	return MENU_OP_INSTANT;
}

uint32_t MenuOpForumMsgView::op_twokeys(std::string parentkey, std::string key)
{


	return MENU_OP_INSTANT;
}

uint32_t MenuOpForumMsgReply::op_twokeys(std::string parentkey, std::string key)
{


	return MENU_OP_INSTANT;
}


uint32_t MenuOpForumMsgWrite::op_twokeys(std::string parentkey, std::string key)
{


	return MENU_OP_INSTANT;
}

/************
 * Shared folders Menu
 */


uint32_t MenuListShared::op()
{
	MenuList::op();

	mList.clear();
	std::list<SharedDirInfo> dirs;
    rsFiles->getSharedDirectories(dirs);
	std::list<SharedDirInfo>::iterator it;
	for(it = dirs.begin(); it != dirs.end(); it++)
	{
		mList.push_back ((*it).virtualname) ;
	}

	return MENU_OP_SUBMENU;
}

int MenuListShared::getEntryDesc(int idx, std::string &desc)
{
	std::list<SharedDirInfo> dirs;
	rsFiles->getSharedDirectories(dirs);
	std::list<SharedDirInfo>::iterator it;
	std::string shareflag;
	int i=0;
	for (it = dirs.begin(); (i < idx) && (it != dirs.end()); it++, i++);
    if (it != dirs.end())
    {
    	if (it->shareflags == (RS_FILE_HINTS_BROWSABLE | RS_FILE_HINTS_NETWORK_WIDE))
    		shareflag = "networkwide - browsable";
    	else if ((it->shareflags & RS_FILE_HINTS_BROWSABLE) == RS_FILE_HINTS_BROWSABLE)
    		shareflag = "private - browsable";
    	else if ((it->shareflags & RS_FILE_HINTS_NETWORK_WIDE) == RS_FILE_HINTS_NETWORK_WIDE)
    		shareflag = "networkwide - anonymous";
    	else
    		shareflag = "not shared";

    	rs_sprintf(desc, "Path: %s Share Type:%s", it->filename.c_str(), shareflag.c_str());
        return MENU_ENTRY_OKAY;
    }
    return MENU_ENTRY_NONE;
}

int MenuListShared::unshareSelected()
{
	if (mSelectIdx < 0)
	{
		std::cout << "MenuListSearchList::unshareSelected() Invalid Selection";
		std::cout << std::endl;
        	return MENU_ENTRY_NONE;
	}
	std::list<SharedDirInfo> dirs;
	rsFiles->getSharedDirectories(dirs);
	std::list<SharedDirInfo>::iterator it;
	int i=0;
	for (it = dirs.begin(); (i < mSelectIdx) && (it != dirs.end()); it++, i++);
    if (it != dirs.end())
    {
    	rsFiles->removeSharedDirectory(it->filename);
        return MENU_ENTRY_OKAY;
    }
    return MENU_ENTRY_NONE;
}

int MenuListShared::toggleFlagSelected(uint32_t shareflags)
{
	if (mSelectIdx < 0)
	{
		std::cout << "MenuListSearchList::unshareSelected() Invalid Selection";
		std::cout << std::endl;
        	return MENU_ENTRY_NONE;
	}
	std::list<SharedDirInfo> dirs;
	rsFiles->getSharedDirectories(dirs);
	std::list<SharedDirInfo>::iterator it;
	int i=0;
	for (it = dirs.begin(); (i < mSelectIdx) && (it != dirs.end()); it++, i++);
    if (it != dirs.end())
    {
    	if((it->shareflags & shareflags) == shareflags)
    	{
        	it->shareflags = it->shareflags & ~shareflags; //disable shareflags
            rsFiles->updateShareFlags(*it);
        } else
        {
        	it->shareflags = it->shareflags | shareflags;  //anable shareflags
            rsFiles->updateShareFlags(*it);
        }
        return MENU_ENTRY_OKAY;
    }
    return MENU_ENTRY_NONE;
}

uint32_t MenuListSharedUnshare::op_basic(std::string key)
{
	Menu* p = parent();
	MenuListShared *mls = dynamic_cast<MenuListShared *>(p);
	if (!mls)
	{
		std::cout << "MenuListShared::op_basic() ERROR";
		std::cout << std::endl;
		return MENU_OP_ERROR;
	}

	mls->unshareSelected();

	return MENU_OP_INSTANT;
}

uint32_t MenuListSharedTogglePublic::op_basic(std::string key)
{
	Menu* p = parent();
	MenuListShared *mls = dynamic_cast<MenuListShared *>(p);
	if (!mls)
	{
		std::cout << "MenuListShared::op_basic() ERROR";
		std::cout << std::endl;
		return MENU_OP_ERROR;
	}

	mls->toggleFlagSelected(RS_FILE_HINTS_NETWORK_WIDE);

	return MENU_OP_INSTANT;
}

uint32_t MenuListSharedToggleBrowsable::op_basic(std::string key)
{
	Menu* p = parent();
	MenuListShared *mls = dynamic_cast<MenuListShared *>(p);
	if (!mls)
	{
		std::cout << "MenuListShared::op_basic() ERROR";
		std::cout << std::endl;
		return MENU_OP_ERROR;
	}

	mls->toggleFlagSelected(RS_FILE_HINTS_BROWSABLE);

	return MENU_OP_INSTANT;
}

uint32_t MenuListSharedAddShare::drawPage(uint32_t drawFlags, std::string &buffer)
{
        buffer += "Enter New path 'path' 'virtualfolder' > ";
	return 1;
}



uint32_t MenuListSharedAddShare::process_lines(std::string input)
{
	/* launch search */
	if (input.size() < 4)
	{
		std::cout << "MenuOpSearchNew::process_lines() ERROR Input too small";
		std::cout << std::endl;
		return MENU_PROCESS_ERROR;
	}

	std::string dir = input.substr(0, input.size() - 1); // remove \n.

	// check that the directory exists!
	if (!RsDirUtil::checkDirectory(dir))
	{
		std::cout << "MenuOpSearchNew::process_lines() ERROR Directory doesn't exist";
		std::cout << std::endl;
		return MENU_PROCESS_ERROR;
	}

	// extract top level as the virtual name.
	std::string topdir = RsDirUtil::getTopDir(input);
	if (topdir.size() < 1)
	{
		std::cout << "MenuOpSearchNew::process_lines() ERROR TopDir is invalid";
		std::cout << std::endl;
		return MENU_PROCESS_ERROR;
	}

	SharedDirInfo shareddir;
	shareddir.filename = dir;
	shareddir.virtualname = topdir;
	shareddir.shareflags = 0x0;

	if (!rsFiles->addSharedDirectory(shareddir))
	{
		std::cout << "MenuOpSearchNew::process_lines() ERROR Adding SharedDir";
		std::cout << std::endl;
		return MENU_PROCESS_ERROR;
	}

	return MENU_PROCESS_DONE;
}
