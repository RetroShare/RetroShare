#include "NotificationHandler.h"

#include <retroshare/rsmsgs.h>

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
	notificationEvent.notificationData.emplace("list", std::to_string(list));
	notificationEvent.notificationData.emplace("type", std::to_string(type));

	eventsVector.push_back(notificationEvent);
}

void NotificationHandler::notifyListChange(int list, int type)
{
	RS_STACK_MUTEX(mMtx); // ********** LOCKED **********
	mStateTokenServer->replaceToken(mStateToken);

	NotificationEvent notificationEvent(LIST_CHANGE);
	notificationEvent.notificationData.emplace("list", std::to_string(list));
	notificationEvent.notificationData.emplace("type", std::to_string(type));

	eventsVector.push_back(notificationEvent);
}

void NotificationHandler::notifyErrorMsg(int list, int sev, std::string msg)
{
	RS_STACK_MUTEX(mMtx); // ********** LOCKED **********
	mStateTokenServer->replaceToken(mStateToken);

	NotificationEvent notificationEvent(ERROR_MSG);
	notificationEvent.notificationData.emplace("list", std::to_string(list));
	notificationEvent.notificationData.emplace("sev", std::to_string(sev));
	notificationEvent.notificationData.emplace("msg", msg);

	eventsVector.push_back(notificationEvent);
}

void NotificationHandler::notifyChatMessage(const ChatMessage& msg)
{
	RS_STACK_MUTEX(mMtx); // ********** LOCKED **********
	mStateTokenServer->replaceToken(mStateToken);

	NotificationEvent notificationEvent(CHAT_MESSAGE);
	notificationEvent.notificationData.emplace("chat_id", msg.chat_id.toStdString());
	notificationEvent.notificationData.emplace("incoming", std::to_string(msg.incoming));

	if(msg.chat_id.isBroadcast())
		notificationEvent.notificationData.emplace("chat_type", "broadcast");
	else if(msg.chat_id.isDistantChatId())
		notificationEvent.notificationData.emplace("chat_type", "distant_chat");
	else if(msg.chat_id.isLobbyId())
		notificationEvent.notificationData.emplace("chat_type", "lobby");
	else if(msg.chat_id.isPeerId())
		notificationEvent.notificationData.emplace("chat_type", "direct_chat");

	eventsVector.push_back(notificationEvent);
}

void NotificationHandler::notifyChatStatus(const ChatId& chat_id, const std::string& status_string)
{
	RS_STACK_MUTEX(mMtx); // ********** LOCKED **********
	mStateTokenServer->replaceToken(mStateToken);

	NotificationEvent notificationEvent(CHAT_STATUS);
	notificationEvent.notificationData.emplace("chat_id", chat_id.toStdString());
	notificationEvent.notificationData.emplace("status_string", status_string);

	eventsVector.push_back(notificationEvent);
}

void NotificationHandler::notifyChatCleared(const ChatId& chat_id)
{
	RS_STACK_MUTEX(mMtx); // ********** LOCKED **********
	mStateTokenServer->replaceToken(mStateToken);

	NotificationEvent notificationEvent(CHAT_CLEARED);
	notificationEvent.notificationData.emplace("chat_id", chat_id.toStdString());

	eventsVector.push_back(notificationEvent);
}

void NotificationHandler::notifyChatLobbyEvent(uint64_t lobby_id, uint32_t event_type, const RsGxsId& nickname, const std::string& any_string)
{
	RS_STACK_MUTEX(mMtx); // ********** LOCKED **********
	mStateTokenServer->replaceToken(mStateToken);

	NotificationEvent notificationEvent(CHAT_LOBBY_EVENT);
	notificationEvent.notificationData.emplace("lobby_id", std::to_string(lobby_id));
	notificationEvent.notificationData.emplace("event_type", std::to_string(event_type));
	notificationEvent.notificationData.emplace("nickname", nickname.toStdString());
	notificationEvent.notificationData.emplace("any_string", any_string);

	eventsVector.push_back(notificationEvent);
}

void NotificationHandler::notifyChatLobbyTimeShift(int time_shift)
{
	RS_STACK_MUTEX(mMtx); // ********** LOCKED **********
	mStateTokenServer->replaceToken(mStateToken);

	NotificationEvent notificationEvent(CHAT_LOBBY_TIME_SHIFT);
	notificationEvent.notificationData.emplace("lobby_id", std::to_string(time_shift));

	eventsVector.push_back(notificationEvent);
}

void NotificationHandler::notifyCustomState(const std::string& peer_id, const std::string& status_string)
{
	RS_STACK_MUTEX(mMtx); // ********** LOCKED **********
	mStateTokenServer->replaceToken(mStateToken);

	NotificationEvent notificationEvent(CUSTOM_STATE);
	notificationEvent.notificationData.emplace("peer_id", peer_id);
	notificationEvent.notificationData.emplace("status_string", status_string);

	eventsVector.push_back(notificationEvent);
}

void NotificationHandler::notifyHashingInfo(uint32_t type, const std::string& fileinfo)
{
	RS_STACK_MUTEX(mMtx); // ********** LOCKED **********
	mStateTokenServer->replaceToken(mStateToken);

	NotificationEvent notificationEvent(HASHING_INFO);
	notificationEvent.notificationData.emplace("type", std::to_string(type));
	notificationEvent.notificationData.emplace("fileinfo", fileinfo);

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
	notificationEvent.notificationData.emplace("peer_id", peer_id);

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
	notificationEvent.notificationData.emplace("location", std::to_string(location));
	notificationEvent.notificationData.emplace("size_limit_in_MB", std::to_string(size_limit_in_MB));

	eventsVector.push_back(notificationEvent);
}

void NotificationHandler::notifyPeerStatusChanged(const std::string& peer_id, uint32_t status)
{
	RS_STACK_MUTEX(mMtx); // ********** LOCKED **********
	mStateTokenServer->replaceToken(mStateToken);

	NotificationEvent notificationEvent(PEER_STATUS_CHANGED);
	notificationEvent.notificationData.emplace("peer_id", peer_id);
	notificationEvent.notificationData.emplace("status", std::to_string(status));

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
	notificationEvent.notificationData.emplace("fileHash", fileHash);

	eventsVector.push_back(notificationEvent);
}

void NotificationHandler::notifyDownloadCompleteCount(uint32_t count)
{
	RS_STACK_MUTEX(mMtx); // ********** LOCKED **********
	mStateTokenServer->replaceToken(mStateToken);

	NotificationEvent notificationEvent(DOWNLOAD_COMPLETE_COUNT);
	notificationEvent.notificationData.emplace("count", std::to_string(count));

	eventsVector.push_back(notificationEvent);
}

void NotificationHandler::notifyHistoryChanged(uint32_t msgId, int type)
{
	RS_STACK_MUTEX(mMtx); // ********** LOCKED **********
	mStateTokenServer->replaceToken(mStateToken);

	NotificationEvent notificationEvent(HISTORY_CHANGED);
	notificationEvent.notificationData.emplace("msgId", std::to_string(msgId));
	notificationEvent.notificationData.emplace("type", std::to_string(type));

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
