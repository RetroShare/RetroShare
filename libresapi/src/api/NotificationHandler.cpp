#include "NotificationHandler.h"

#include <sstream>

#include <retroshare/rsmsgs.h>

template<typename T>
std::string toString(const T& value)
{
	std::ostringstream oss;
	oss << value;
	return oss.str();
}

namespace resource_api
{
NotificationHandler::NotificationHandler(StateTokenServer* sts, RsNotify* notify) :
    mStateTokenServer(sts),
    mNotify(notify),
    mMtx("NotificationHandler Mtx"),
    mStateToken(sts->getNewToken())
{
	mNotify->registerNotifyClient(this);

	addResourceHandler("*", this, &NotificationHandler::handleWildcard);
}

NotificationHandler::~NotificationHandler()
{
	mNotify->unregisterNotifyClient(this);
}

void NotificationHandler::notifyListPreChange(int list, int type)
{
	RS_STACK_MUTEX(mMtx); // ********** LOCKED **********
	mStateTokenServer->replaceToken(mStateToken);

	NotificationEvent notificationEvent(LIST_PRE_CHANGE);

	notificationEvent.notificationData.insert(std::pair<std::string, std::string>("list", toString(list)));
	notificationEvent.notificationData.insert(std::pair<std::string, std::string>("type", toString(type)));

	eventsVector.push_back(notificationEvent);
}

void NotificationHandler::notifyListChange(int list, int type)
{
	RS_STACK_MUTEX(mMtx); // ********** LOCKED **********
	mStateTokenServer->replaceToken(mStateToken);

	NotificationEvent notificationEvent(LIST_CHANGE);
	notificationEvent.notificationData.insert(std::pair<std::string, std::string>("list", toString(list)));
	notificationEvent.notificationData.insert(std::pair<std::string, std::string>("type", toString(type)));

	eventsVector.push_back(notificationEvent);
}

void NotificationHandler::notifyErrorMsg(int list, int sev, std::string msg)
{
	RS_STACK_MUTEX(mMtx); // ********** LOCKED **********
	mStateTokenServer->replaceToken(mStateToken);

	NotificationEvent notificationEvent(ERROR_MSG);
	notificationEvent.notificationData.insert(std::pair<std::string, std::string>("list", toString(list)));
	notificationEvent.notificationData.insert(std::pair<std::string, std::string>("sev", toString(sev)));
	notificationEvent.notificationData.insert(std::pair<std::string, std::string>("msg", msg));

	eventsVector.push_back(notificationEvent);
}

void NotificationHandler::notifyChatMessage(const ChatMessage& msg)
{
	RS_STACK_MUTEX(mMtx); // ********** LOCKED **********
	mStateTokenServer->replaceToken(mStateToken);

	NotificationEvent notificationEvent(CHAT_MESSAGE);
	notificationEvent.notificationData.insert(std::pair<std::string, std::string>("chat_id", msg.chat_id.toStdString()));
	notificationEvent.notificationData.insert(std::pair<std::string, std::string>("incoming", toString(msg.incoming)));

	if(msg.chat_id.isBroadcast())
		notificationEvent.notificationData.insert(std::pair<std::string, std::string>("chat_type", "broadcast"));
	else if(msg.chat_id.isDistantChatId())
		notificationEvent.notificationData.insert(std::pair<std::string, std::string>("chat_type", "distant_chat"));
	else if(msg.chat_id.isLobbyId())
		notificationEvent.notificationData.insert(std::pair<std::string, std::string>("chat_type", "lobby"));
	else if(msg.chat_id.isPeerId())
		notificationEvent.notificationData.insert(std::pair<std::string, std::string>("chat_type", "direct_chat"));

	eventsVector.push_back(notificationEvent);
}

void NotificationHandler::notifyChatStatus(const ChatId& chat_id, const std::string& status_string)
{
	RS_STACK_MUTEX(mMtx); // ********** LOCKED **********
	mStateTokenServer->replaceToken(mStateToken);

	NotificationEvent notificationEvent(CHAT_STATUS);
	notificationEvent.notificationData.insert(std::pair<std::string, std::string>("chat_id", chat_id.toStdString()));
	notificationEvent.notificationData.insert(std::pair<std::string, std::string>("status_string", status_string));

	eventsVector.push_back(notificationEvent);
}

void NotificationHandler::notifyChatCleared(const ChatId& chat_id)
{
	RS_STACK_MUTEX(mMtx); // ********** LOCKED **********
	mStateTokenServer->replaceToken(mStateToken);

	NotificationEvent notificationEvent(CHAT_CLEARED);
	notificationEvent.notificationData.insert(std::pair<std::string, std::string>("chat_id", chat_id.toStdString()));

	eventsVector.push_back(notificationEvent);
}

void NotificationHandler::notifyChatLobbyEvent(uint64_t lobby_id, uint32_t event_type, const RsGxsId& nickname, const std::string& any_string)
{
	RS_STACK_MUTEX(mMtx); // ********** LOCKED **********
	mStateTokenServer->replaceToken(mStateToken);

	NotificationEvent notificationEvent(CHAT_LOBBY_EVENT);
	notificationEvent.notificationData.insert(std::pair<std::string, std::string>("lobby_id", toString(lobby_id)));
	notificationEvent.notificationData.insert(std::pair<std::string, std::string>("event_type", toString(event_type)));
	notificationEvent.notificationData.insert(std::pair<std::string, std::string>("nickname", nickname.toStdString()));
	notificationEvent.notificationData.insert(std::pair<std::string, std::string>("any_string", any_string));

	eventsVector.push_back(notificationEvent);
}

void NotificationHandler::notifyChatLobbyTimeShift(int time_shift)
{
	RS_STACK_MUTEX(mMtx); // ********** LOCKED **********
	mStateTokenServer->replaceToken(mStateToken);

	NotificationEvent notificationEvent(CHAT_LOBBY_TIME_SHIFT);
	notificationEvent.notificationData.insert(std::pair<std::string, std::string>("lobby_id", toString(time_shift)));

	eventsVector.push_back(notificationEvent);
}

void NotificationHandler::notifyCustomState(const std::string& peer_id, const std::string& status_string)
{
	RS_STACK_MUTEX(mMtx); // ********** LOCKED **********
	mStateTokenServer->replaceToken(mStateToken);

	NotificationEvent notificationEvent(CUSTOM_STATE);
	notificationEvent.notificationData.insert(std::pair<std::string, std::string>("peer_id", peer_id));
	notificationEvent.notificationData.insert(std::pair<std::string, std::string>("status_string", status_string));

	eventsVector.push_back(notificationEvent);
}

void NotificationHandler::notifyHashingInfo(uint32_t type, const std::string& fileinfo)
{
	RS_STACK_MUTEX(mMtx); // ********** LOCKED **********
	mStateTokenServer->replaceToken(mStateToken);

	NotificationEvent notificationEvent(HASHING_INFO);
	notificationEvent.notificationData.insert(std::pair<std::string, std::string>("type", toString(type)));
	notificationEvent.notificationData.insert(std::pair<std::string, std::string>("fileinfo", fileinfo));

	eventsVector.push_back(notificationEvent);
}

void NotificationHandler::notifyTurtleSearchResult(uint32_t search_id, const std::list<TurtleFileInfo>& files)
{
	RS_STACK_MUTEX(mMtx); // ********** LOCKED **********
	mStateTokenServer->replaceToken(mStateToken);

	NotificationEvent notificationEvent(TURTLE_SEARCH_RESULT);

	eventsVector.push_back(notificationEvent);
}

void NotificationHandler::notifyPeerHasNewAvatar(std::string peer_id)
{
	RS_STACK_MUTEX(mMtx); // ********** LOCKED **********
	mStateTokenServer->replaceToken(mStateToken);

	NotificationEvent notificationEvent(PEER_HAS_NEW_AVATAR);
	notificationEvent.notificationData.insert(std::pair<std::string, std::string>("peer_id", peer_id));

	eventsVector.push_back(notificationEvent);
}

void NotificationHandler::notifyOwnAvatarChanged()
{
	RS_STACK_MUTEX(mMtx); // ********** LOCKED **********
	mStateTokenServer->replaceToken(mStateToken);

	NotificationEvent notificationEvent(OWN_AVATAR_CHANGED);

	eventsVector.push_back(notificationEvent);
}

void NotificationHandler::notifyOwnStatusMessageChanged()
{
	RS_STACK_MUTEX(mMtx); // ********** LOCKED **********
	mStateTokenServer->replaceToken(mStateToken);

	NotificationEvent notificationEvent(OWN_STATUS_MESSAGE_CHANGED);

	eventsVector.push_back(notificationEvent);
}

void NotificationHandler::notifyDiskFull(uint32_t location, uint32_t size_limit_in_MB)
{
	RS_STACK_MUTEX(mMtx); // ********** LOCKED **********
	mStateTokenServer->replaceToken(mStateToken);

	NotificationEvent notificationEvent(DISK_FULL);
	notificationEvent.notificationData.insert(std::pair<std::string, std::string>("location", toString(location)));
	notificationEvent.notificationData.insert(std::pair<std::string, std::string>("size_limit_in_MB", toString(size_limit_in_MB)));

	eventsVector.push_back(notificationEvent);
}

void NotificationHandler::notifyPeerStatusChanged(const std::string& peer_id, uint32_t status)
{
	RS_STACK_MUTEX(mMtx); // ********** LOCKED **********
	mStateTokenServer->replaceToken(mStateToken);

	NotificationEvent notificationEvent(PEER_STATUS_CHANGED);
	notificationEvent.notificationData.insert(std::pair<std::string, std::string>("peer_id", peer_id));
	notificationEvent.notificationData.insert(std::pair<std::string, std::string>("status", toString(status)));

	eventsVector.push_back(notificationEvent);
}

void NotificationHandler::notifyGxsChange(const RsGxsChanges& changes)
{
	RS_STACK_MUTEX(mMtx); // ********** LOCKED **********
	mStateTokenServer->replaceToken(mStateToken);

	NotificationEvent notificationEvent(GXS_CHANGE);

	eventsVector.push_back(notificationEvent);
}

void NotificationHandler::notifyPeerStatusChangedSummary()
{
	RS_STACK_MUTEX(mMtx); // ********** LOCKED **********
	mStateTokenServer->replaceToken(mStateToken);

	NotificationEvent notificationEvent(PEER_STATUS_CHANGED_SUMMARY);

	eventsVector.push_back(notificationEvent);
}

void NotificationHandler::notifyDiscInfoChanged()
{
	RS_STACK_MUTEX(mMtx); // ********** LOCKED **********
	mStateTokenServer->replaceToken(mStateToken);

	NotificationEvent notificationEvent(DISC_INFO_CHANGED);

	eventsVector.push_back(notificationEvent);
}

void NotificationHandler::notifyDownloadComplete(const std::string& fileHash)
{
	RS_STACK_MUTEX(mMtx); // ********** LOCKED **********
	mStateTokenServer->replaceToken(mStateToken);

	NotificationEvent notificationEvent(DOWNLOAD_COMPLETE);
	notificationEvent.notificationData.insert(std::pair<std::string, std::string>("fileHash", fileHash));

	eventsVector.push_back(notificationEvent);
}

void NotificationHandler::notifyDownloadCompleteCount(uint32_t count)
{
	RS_STACK_MUTEX(mMtx); // ********** LOCKED **********
	mStateTokenServer->replaceToken(mStateToken);

	NotificationEvent notificationEvent(DOWNLOAD_COMPLETE_COUNT);
	notificationEvent.notificationData.insert(std::pair<std::string, std::string>("count", toString(count)));

	eventsVector.push_back(notificationEvent);
}

void NotificationHandler::notifyHistoryChanged(uint32_t msgId, int type)
{
	RS_STACK_MUTEX(mMtx); // ********** LOCKED **********
	mStateTokenServer->replaceToken(mStateToken);

	NotificationEvent notificationEvent(HISTORY_CHANGED);
	notificationEvent.notificationData.insert(std::pair<std::string, std::string>("msgId", toString(msgId)));
	notificationEvent.notificationData.insert(std::pair<std::string, std::string>("type", toString(type)));

	eventsVector.push_back(notificationEvent);
}

void NotificationHandler::handleWildcard(Request& req, Response& resp)
{
	RS_STACK_MUTEX(mMtx); // ********** LOCKED **********

	if(!eventsVector.empty())
	{
		StreamBase& eventsStream = resp.mDataStream.getStreamToMember("events");
		eventsStream.getStreamToMember();
		for(std::vector<NotificationEvent>::iterator it = eventsVector.begin(); it != eventsVector.end(); it++)
		{
			StreamBase& eventStream = eventsStream.getStreamToMember();
			eventStream << makeKeyValue("eventType", (int)(*it).notificationType);

			for(std::map<std::string, std::string>::iterator mapIt = (*it).notificationData.begin(); mapIt != (*it).notificationData.end(); mapIt++)
			{
				eventStream << makeKeyValueReference(mapIt->first, mapIt->second);
			}
		}
	}

	resp.mStateToken = mStateToken;
	resp.setOk();
	eventsVector.clear();
}

} // namespace resource_api
