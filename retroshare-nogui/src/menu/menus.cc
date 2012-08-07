
#include <retroshare/rspeers.h>
#include <retroshare/rsfiles.h>
#include <retroshare/rsturtle.h>
#include "util/rsstring.h"

#include "menu/menus.h"

// Can't use: Q, H, T, U (universal)
// or L, N, P (list ops).
// or 0-9,a-f (list selection).

#define 	MENU_FRIENDS_KEY_ADD  			'A'
#define 	MENU_FRIENDS_KEY_VIEW  			'V'
#define 	MENU_FRIENDS_KEY_REMOVE  		'D'
#define 	MENU_FRIENDS_KEY_CHAT  			'C'

#define 	MENU_TRANSFER_KEY_STOP  		'S'
#define 	MENU_TRANSFER_KEY_CANCEL  		'C'

#define 	MENU_SEARCH_KEY_ADD  			'A'
#define 	MENU_SEARCH_KEY_REMOVE  		'D'
#define 	MENU_SEARCH_KEY_VIEW  			'V'
#define 	MENU_SEARCH_KEY_DOWNLOAD  		'G'

#define 	MENU_FRIENDS_KEY_ADD  			'A'
#define 	MENU_FRIENDS_KEY_VIEW  			'V'
#define 	MENU_FRIENDS_KEY_REMOVE  		'D'
#define 	MENU_FRIENDS_KEY_CHAT  			'C'


#define 	MENU_TOPLEVEL_KEY_FRIENDS  		'F'
#define 	MENU_TOPLEVEL_KEY_NETWORK  		'W'
#define 	MENU_TOPLEVEL_KEY_TRANSFER  		'D'
#define 	MENU_TOPLEVEL_KEY_SEARCH  		'S'
#define 	MENU_TOPLEVEL_KEY_FORUMS  		'O'


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
	search->addMenuItem(MENU_SEARCH_KEY_ADD, new MenuOpSearchNew());
	//search->addMenuItem(MENU_SEARCH_KEY_REMOVE, new MenuOpSearchDelete());
	search->addMenuItem(MENU_SEARCH_KEY_VIEW, searchlist);
	searchlist->addMenuItem(MENU_SEARCH_KEY_DOWNLOAD, new MenuOpSearchListDownload());

	// Forums - TODO.
	//MenuList *forums = new MenuListForums();
	//forums->addMenuItem(MENU_FRIENDS_KEY_ADD, new MenuOpFriendsAdd());
	//forums->addMenuItem(MENU_FRIENDS_KEY_VIEW, new MenuOpFriendsViewDetail());
	//forums->addMenuItem(MENU_FRIENDS_KEY_REMOVE, new MenuOpFriendsRemove());
	//forums->addMenuItem(MENU_FRIENDS_KEY_CHAT, new MenuOpFriendsChat());


	/* Top Level Menu */
	Menu *tlm = new Menu("Top Level Menu");

	tlm->addMenuItem(MENU_TOPLEVEL_KEY_FRIENDS, friends);
	tlm->addMenuItem(MENU_TOPLEVEL_KEY_NETWORK, network);
	tlm->addMenuItem(MENU_TOPLEVEL_KEY_TRANSFER, transfers);
	tlm->addMenuItem(MENU_TOPLEVEL_KEY_SEARCH, search);
	//tlm->addMenuItem(MENU_TOPLEVEL_KEY_FORUMS, forums);

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
		std::cerr << "MenuListTransfer::getEntryDesc() No ListEntry";
		std::cerr << std::endl;
		return 0;
	}

	FileInfo info;
	if (!rsFiles->FileDetails(hash, RS_FILE_HINTS_DOWNLOAD, info))
	{
		std::cerr << "MenuListTransfer::getEntryDesc() No FileDetails";
		std::cerr << std::endl;
		return 0;
	}

	float frac = (float) info.transfered / info.size;

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
	std::cerr << "MenuListSearch::storeSearch(" << searchId << " => ";
	std::cerr << match_string;
	std::cerr << std::endl;

	std::string strSearchId;
	rs_sprintf(strSearchId, "%lu", searchId);
	mList.push_back(strSearchId);

	mSearchTerms[strSearchId] = match_string;
	mSearchIds[strSearchId] = searchId;

	return 1;
}

int MenuListSearch::removeSearch(std::string strSearchId)
{
	std::cerr << "MenuListSearch::removeSearch(" << strSearchId << ")";
	std::cerr << std::endl;

	std::map<std::string, uint32_t>::iterator it;
	it = mSearchIds.find(strSearchId);
	if (it != mSearchIds.end())
	{
		/* cleanup local maps */

		/* cancel search */

		/* clear results from Notify Collector */
	}

	return 1;
}


uint32_t MenuOpSearchNew::process_lines(std::string input)
{
	/* launch search */
	if (input.size() < 4)
	{
		std::cerr << "MenuOpSearchNew::process_lines() ERROR Input too small";
		std::cerr << std::endl;
		return MENU_PROCESS_ERROR;
	}

	std::string search = input.substr(0, input.size() - 1); // remove \n.
	uint32_t searchId = (uint32_t) rsTurtle->turtleSearch(search);

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
		std::cerr << "MenuListSearchList::refresh() mls not there";
		std::cerr << std::endl;
		return MENU_OP_ERROR;
	}

	/* load friend list*/
	mList.clear();

	uint32_t searchId;
	if (!mls->getCurrentSearchId(searchId))
	{
		std::cerr << "MenuListSearchList::refresh() currentIdx invalid";
		std::cerr << std::endl;
		return MENU_OP_ERROR;
	}

	std::cerr << "MenuListSearchList::refresh() searchId: " << searchId;
	std::cerr << std::endl;

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
		std::cerr << "MenuListSearchList::downloadSelected() Invalid Selection";
		std::cerr << std::endl;
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
			std::cerr << "MenuListSearchList::downloadSelected() Download Started";
			std::cerr << std::endl;
		}
		else
		{
			std::cerr << "MenuListSearchList::downloadSelected() Error Starting Download";
			std::cerr << std::endl;
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
		std::cerr << "MenuOpSearchListDownload::op_basic() ERROR";
		std::cerr << std::endl;
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



