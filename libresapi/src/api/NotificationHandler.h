#ifndef NOTIFICATIONHANDLER_H
#define NOTIFICATIONHANDLER_H

#include <retroshare/rsnotify.h>
#include <util/rsthreads.h>

#include "ResourceRouter.h"
#include "StateTokenServer.h"

#include "JsonStream.h"

namespace resource_api
{
class NotificationHandler : public ResourceRouter, NotifyClient
{
public:
	enum NotificationType{
		LIST_PRE_CHANGE,
		LIST_CHANGE,
		ERROR_MSG,
		CHAT_MESSAGE,
		CHAT_STATUS,
		CHAT_CLEARED,
		CHAT_LOBBY_EVENT,
		CHAT_LOBBY_TIME_SHIFT,
		CUSTOM_STATE,
		HASHING_INFO,
		TURTLE_SEARCH_RESULT,
		PEER_HAS_NEW_AVATAR,
		OWN_AVATAR_CHANGED,
		OWN_STATUS_MESSAGE_CHANGED,
		DISK_FULL,
		PEER_STATUS_CHANGED,
		GXS_CHANGE,
		PEER_STATUS_CHANGED_SUMMARY,
		DISC_INFO_CHANGED,
		DOWNLOAD_COMPLETE,
		DOWNLOAD_COMPLETE_COUNT,
		HISTORY_CHANGED
	};

	NotificationHandler(StateTokenServer* sts, RsNotify* notify);
	virtual ~NotificationHandler();

	virtual void notifyListPreChange(int list, int type);
	virtual void notifyListChange(int list, int type);
	virtual void notifyErrorMsg(int list, int sev, std::string msg);
	virtual void notifyChatMessage(const ChatMessage& msg);
	virtual void notifyChatStatus(const ChatId& chat_id, const std::string& status_string);
	virtual void notifyChatCleared(const ChatId& chat_id);
	virtual void notifyChatLobbyEvent(uint64_t lobby_id, uint32_t event_type, const RsGxsId& nickname, const std::string& any_string);
	virtual void notifyChatLobbyTimeShift(int time_shift);
	virtual void notifyCustomState(const std::string& peer_id, const std::string& status_string);
	virtual void notifyHashingInfo(uint32_t type, const std::string& fileinfo);
	virtual void notifyTurtleSearchResult(uint32_t search_id, const std::list<TurtleFileInfo>& files);
	virtual void notifyPeerHasNewAvatar(std::string peer_id);
	virtual void notifyOwnAvatarChanged();
	virtual void notifyOwnStatusMessageChanged();
	virtual void notifyDiskFull(uint32_t location, uint32_t size_limit_in_MB);
	virtual void notifyPeerStatusChanged(const std::string& peer_id, uint32_t status);
	virtual void notifyGxsChange(const RsGxsChanges& changes);

	/* one or more peers has changed the states */
	virtual void notifyPeerStatusChangedSummary();
	virtual void notifyDiscInfoChanged();

	virtual void notifyDownloadComplete(const std::string& fileHash);
	virtual void notifyDownloadCompleteCount(uint32_t count);
	virtual void notifyHistoryChanged(uint32_t msgId, int type);

private:
	void handleWildcard(Request& req, Response& resp);

	StateTokenServer* mStateTokenServer;
	RsNotify* mNotify;

	RsMutex mMtx;
	StateToken mStateToken; // mutex protected

	struct NotificationEvent {
		NotificationEvent(NotificationType nEventType) :
		    notificationType(nEventType)
		{}

		NotificationType notificationType;
		std::map<std::string, std::string> notificationData;
	};

	std::vector<NotificationEvent> eventsVector;
};
} // namespace resource_api

#endif // NOTIFICATIONHANDLER_H
