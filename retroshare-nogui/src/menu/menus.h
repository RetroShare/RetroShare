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

#include "menu/menu.h"
#include <retroshare/rsturtle.h>
#include "notifytxt.h" /* needed to access search results */
#include <map>

Menu *CreateMenuStructure(NotifyTxt *notify);

/************
 * Friends Menu
 */

class MenuListFriends: public MenuList
{

public:
	MenuListFriends() :MenuList("Friends") { return; }

	virtual uint32_t op();
	int getEntryDesc(int idx, std::string &desc);

protected:
	MenuListFriends(std::string name) :MenuList(name) { return; } // For Network.
};


class MenuListNetwork: public MenuListFriends
{

public:
	MenuListNetwork() :MenuListFriends("Network") { return; }

	virtual uint32_t op();
};


class MenuOpFriendsAdd: public MenuOpBasicKey
{
	public:

	MenuOpFriendsAdd() :MenuOpBasicKey("Add") { return; }
	virtual uint32_t op_basic(std::string hash);
};


class MenuOpFriendsViewDetail: public MenuOpBasicKey
{
	public:

	MenuOpFriendsViewDetail() :MenuOpBasicKey("View") { return; }
	virtual uint32_t op_basic(std::string hash);
};


class MenuOpFriendsRemove: public MenuOpBasicKey
{
	public:

	MenuOpFriendsRemove() :MenuOpBasicKey("Remove") { return; }
	virtual uint32_t op_basic(std::string hash);
};


class MenuOpFriendsChat: public MenuOpBasicKey
{
	public:

	MenuOpFriendsChat() :MenuOpBasicKey("Chat") { return; }
	virtual uint32_t op_basic(std::string hash);
};



/************
 * Transfer Menu
 */


class MenuListTransfer: public MenuList
{
	public:

	MenuListTransfer() :MenuList("Downloads") { return; }
	virtual uint32_t op();
	int getEntryDesc(int idx, std::string &desc);
};


class MenuOpTransferStop: public MenuOpBasicKey
{
	public:

	MenuOpTransferStop() :MenuOpBasicKey("Stop") { return; }
	virtual uint32_t op_basic(std::string hash);
};



class MenuOpTransferCancel: public MenuOpBasicKey
{
	public:

	MenuOpTransferCancel() :MenuOpBasicKey("Cancel") { return; }
	virtual uint32_t op_basic(std::string hash);
};



/************
 * Search Menu
 */


class MenuListSearch: public MenuList
{
	public:

	MenuListSearch(NotifyTxt *notify) 
	:MenuList("Search"), mNotify(notify) { return; }

	virtual uint32_t op();
	int getEntryDesc(int idx, std::string &desc);

	/* specific Search Functions */
	int getCurrentSearchId(uint32_t &id);
	int storeSearch(uint32_t searchId, std::string match_string);
	int removeSearch(std::string strSearchId);

private:
	std::map<std::string, uint32_t> mSearchIds;
	std::map<std::string, std::string> mSearchTerms;
	NotifyTxt *mNotify;
};


class MenuOpSearchNew: public MenuOpLineInput
{
	public:

	MenuOpSearchNew(NotifyTxt *notify) 
	:MenuOpLineInput("New"), mNotify(notify) { return; }
	virtual uint32_t process_lines(std::string input);
	virtual uint32_t drawPage(uint32_t drawFlags, std::string &buffer);

private:
	NotifyTxt *mNotify;
};


class MenuOpSearchDelete: public MenuOpBasicKey
{
	public:

	MenuOpSearchDelete() :MenuOpBasicKey("Delete") { return; }
	virtual uint32_t op_basic(std::string hash);
};


class MenuListSearchList: public MenuList
{
	public:

	MenuListSearchList(NotifyTxt *notify) 
	:MenuList("Results"), mNotify(notify) { return; }
	virtual uint32_t op();
	uint32_t refresh();
	int getEntryDesc(int idx, std::string &desc);
	int downloadSelected();

	private:
	NotifyTxt *mNotify;
	std::list<TurtleFileInfo> mSearchResults; // local cache for consistency.
};


class MenuOpSearchListDownload: public MenuOpBasicKey
{
	public:

	MenuOpSearchListDownload() :MenuOpBasicKey("Download") { return; }
	virtual uint32_t op_basic(std::string hash);
};



/************
 * Forums Menu
 */



class MenuListForums: public MenuList
{
	public:

	MenuListForums() :MenuList("Forums SubMenu") { return; }
	virtual uint32_t op();
	int getEntryDesc(int idx, std::string &desc);
};


class MenuOpForumDetails: public MenuOpBasicKey
{
	public:

	MenuOpForumDetails() :MenuOpBasicKey("Show Forum Details") { return; }
	virtual uint32_t op_basic(std::string hash);
};


class MenuOpForumSubscribe: public MenuOpBasicKey
{
	public:

	MenuOpForumSubscribe() :MenuOpBasicKey("Subscribe To Forum") { return; }
	virtual uint32_t op_basic(std::string hash);
};


class MenuOpForumUnsubscribe: public MenuOpBasicKey
{
	public:

	MenuOpForumUnsubscribe() :MenuOpBasicKey("Unsubscribe To Forum") { return; }
	virtual uint32_t op_basic(std::string hash);
};


class MenuOpForumCreate: public MenuOpBasicKey
{
	public:

	MenuOpForumCreate() :MenuOpBasicKey("Create Forum") { return; }
	virtual uint32_t op_basic(std::string hash);
};



class MenuListForumMsgs: public MenuList
{
	public:

	MenuListForumMsgs() :MenuList("List Forum Msgs") { return; }
	virtual uint32_t op();
	int getEntryDesc(int idx, std::string &desc);
};

class MenuOpForumMsgView: public MenuOpTwoKeys
{
	public:

	MenuOpForumMsgView() :MenuOpTwoKeys("View Message") { return; }
	virtual uint32_t op_twokeys(std::string parentkey, std::string key);
};


class MenuOpForumMsgReply: public MenuOpTwoKeys
{
	public:

	MenuOpForumMsgReply() :MenuOpTwoKeys("Reply to Message") { return; }
	virtual uint32_t op_twokeys(std::string parentkey, std::string key);
};


class MenuOpForumMsgWrite: public MenuOpTwoKeys
{
	public:

	MenuOpForumMsgWrite() :MenuOpTwoKeys("Write Message") { return; }
	virtual uint32_t op_twokeys(std::string parentkey, std::string key);
};

/************
 * Shared folders Menu
 */


class MenuListShared: public MenuList
{
	public:

	MenuListShared() :MenuList("My Shared Directories") { return; }
	virtual uint32_t op();
	int getEntryDesc(int idx, std::string &desc);
	int unshareSelected();
	int toggleFlagSelected(uint32_t shareflags);
};



class MenuListSharedUnshare: public MenuOpBasicKey
{
	public:

	MenuListSharedUnshare() :MenuOpBasicKey("Stop Sharing Selected") { return; }
	virtual uint32_t op_basic(std::string key);
};

class MenuListSharedTogglePublic: public MenuOpBasicKey
{
	public:

	MenuListSharedTogglePublic() :MenuOpBasicKey("Enable/Disable Networkwide Sharing") { return; }
	virtual uint32_t op_basic(std::string key);
};

class MenuListSharedToggleBrowsable: public MenuOpBasicKey
{
	public:

	MenuListSharedToggleBrowsable() :MenuOpBasicKey("Enable/Disable Browsing Of Selected") { return; }
	virtual uint32_t op_basic(std::string key);
};

class MenuListSharedAddShare: public MenuOpLineInput
{
	public:

	MenuListSharedAddShare() :MenuOpLineInput("Add new Share") { return; }
	virtual uint32_t process_lines(std::string input);
	virtual uint32_t drawPage(uint32_t drawFlags, std::string &buffer);
};

