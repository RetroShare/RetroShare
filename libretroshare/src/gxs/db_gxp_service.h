
/* Generalised Service Base Class.
 * The base class interfaces with gixp, disk, net services.
 */

class RsGxpLink
{
	uint32_t type;
	std::string msgId;
}

class RsGxpItem
{
	std::string msgId;
	std::string origMsgId;
	std::string groupId;
	gxpTime		time;
	std::list<std::string> hashtags;
	std::list<RsGxpLink> linked;

        RsGxpSignature mSignature;
};

class RsGxpGroup
{



};

class gxp_service
{
	public:

	requestItem(msgId);

	recv(RsGxpItem *item);
	send(RsGxpItem *item);

	/****************************************************************************************************/
	// Event Callback for the service.
	/****************************************************************************************************/

	notify_groupChanged();  // Mainly for GUI display.
	notify_newMessage(); // used for newsFeeds.	
	notify_duplicateMessage(); // Channels needs this for Downloading stuff, can probably be moved above.
	locked_checkDistribMsg(); // required overload?
	/****************************************************************************************************/
	// Must worry about sharing keys.
	// Can gixp handle it? 
	// What interfaces do we need here?
	/****************************************************************************************************/

	/****************************************************************************************************/
	// Interface for Message Read Status
	// At the moment this is overloaded to handle autodownload flags too.
	// This is a configuration thing.
	/****************************************************************************************************/

	int flagItemRead(std::string id);
	int flagItemUnread(std::string id);
	int flagGroupRead(std::string id);
	int flagGroupUnread(std::string id);

	/****************************************************************************************************/
	// External Interface for Data.
	/****************************************************************************************************/

	// Mesage Creation.
	int createGroup(RsGxpGroup *grp);
	int createItem(RsGxpItem *item);

	// Group Lists & Message Lists.
	int getGroupsChanged(std::list<std::string> &groupIds);
	int getGroups(std::list<std::string> &groupIds);
	int getGroupList(std::string grpId, std::list<std::string> &groupIds);
	int getTimeRange(GxpTime from, GxpTime to, std::list<std::string> &msgIds);
	int getGroupTimeRange(std::string grpId, GxpTime from, GxpTime to, std::list<std::string> &msgIds);
	int getReplacementMsgs(std::string origId, std::list<std::string> replaceIds);

	// Getting the Actual Data.
	int haveItem(std::string msgId);
	int requestItem(std::string msgId);
	RsGxpItem  *getMsg_locked(std::string msgId); 
	RsGxpGroup *getGroup_locked(std::string msgId); 

	// Interface with GIXP Stuff... everything else should be behind the scenes.
	RsGixpProfile *getProfile_locked(std::string id);

	// Immediate Search...
	int searchReferringLinks(RsGxpLink link, std::list<std::string> &msgIds);
	int searchHashTags(std::list<std::string>, std::list<std::string> &msgIds);

	// Remote Search...

	private:

	// Reversed Index: for hash searching.
	std::map<std::string, std::list<ids> > mSearchMap;
};




