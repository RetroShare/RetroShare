#ifndef RSIFACE_NOTIFY_TXT_H
#define RSIFACE_NOTIFY_TXT_H

#include <retroshare/rsiface.h>
#include <retroshare/rsturtle.h>
#include <retroshare/rsnotify.h>
#include <retroshare/rsmsgs.h>
#include <QObject>
#include <QMutex>
#include <QPoint>
//#include <QMutex>

#include <string>

class QTimer;
class NetworkDialog;
class FriendsDialog;
class SharedFilesDialog;
class TransfersDialog;
class ChatDialog;
class MessagesDialog;
class ChannelsDialog;
class MessengerWindow;
class ToasterItem;
class ToasterNotify;
class SignatureEventData ;
struct TurtleFileInfo;

class NotifyQt: public QObject, public NotifyClient
{
	Q_OBJECT
	public:
		static NotifyQt *Create ();
		static NotifyQt *getInstance ();
		static bool isAllDisable();
		void enable() ;

		virtual ~NotifyQt() { return; }

		void setNetworkDialog(NetworkDialog *c) { cDialog = c; }

		virtual void notifyListPreChange(int list, int type);
		virtual void notifyListChange(int list, int type);
		virtual void notifyErrorMsg(int list, int sev, std::string msg);
		virtual void notifyChatMessage(const ChatMessage&        /* msg */);
		virtual void notifyChatStatus(const ChatId &chat_id,const std::string& status_string);
		virtual void notifyChatCleared(const ChatId &chat_id);
		virtual void notifyCustomState(const std::string& peer_id, const std::string& status_string);
		virtual void notifyHashingInfo(uint32_t type, const std::string& fileinfo);
		virtual void notifyTurtleSearchResult(uint32_t search_id,const std::list<TurtleFileInfo>& found_files);
		virtual void notifyPeerHasNewAvatar(std::string peer_id) ;
		virtual void notifyOwnAvatarChanged() ;
        virtual void notifyChatLobbyEvent(uint64_t /* lobby id */, uint32_t /* event type */, const RsGxsId & /*nickname*/, const std::string& /* any string */) ;
		virtual void notifyChatLobbyTimeShift(int time_shift) ;

		virtual void notifyOwnStatusMessageChanged() ;
		virtual void notifyDiskFull(uint32_t loc,uint32_t size_in_mb) ;
		/* peer has changed the state */
		virtual void notifyPeerStatusChanged(const std::string& peer_id, uint32_t state);
		/* one or more peers has changed the states */
		virtual void notifyPeerStatusChangedSummary();

        virtual void notifyGxsChange(const RsGxsChanges& change);

		virtual void notifyHistoryChanged(uint32_t msgId, int type);

		virtual void notifyDiscInfoChanged() ;
		virtual void notifyDownloadComplete(const std::string& fileHash);
		virtual void notifyDownloadCompleteCount(uint32_t count);
        virtual bool askForPassword(const std::string& key_details, bool prev_is_bad, std::string& password, bool &cancelled);
		virtual bool askForPluginConfirmation(const std::string& plugin_filename, const std::string& plugin_file_hash);

		// Queues the signature event so that it canhappen in the main GUI thread (to ask for passwd).
		// To use this function: call is multiple times as soon as it returns true.
		//
		// Dont' use a while, if you're in a mutexed part, otherwize it will lock. You need to call the function
		// and periodically exit the locked code between calls to allow the signature to happen.
		//
		// Returns:
		// 	false = the signature is registered, but the result is not there yet. Call again soon.
		// 	true  = signature done. Data is ready. signature_result takes the following values:
		// 					1: signature success
		// 					2: signature failed. Wrong passwd, user pressed cancel, etc.
		//
		virtual bool askForDeferredSelfSignature(const void *data, const uint32_t len, unsigned char *sign, unsigned int *signlen,int& signature_result) ;

		/* Notify from GUI */
		void notifyChatFontChanged();
		void notifyChatStyleChanged(int /*ChatStyle::enumStyleType*/ styleType);

		void testToasters(uint notifyFlags, /*RshareSettings::enumToasterPosition*/ int position, QPoint margin);
		void testToaster(ToasterNotify *toasterNotify, /*RshareSettings::enumToasterPosition*/ int position, QPoint margin);
		void testToaster(QString tag, ToasterNotify *toasterNotify, /*RshareSettings::enumToasterPosition*/ int position, QPoint margin);

		void notifySettingsChanged();

	signals:
		// It's beneficial to send info to the GUI using signals, because signals are thread-safe
		// as they get queued by Qt.
		//
		void hashingInfoChanged(const QString&) const ;
		void filesPreModChanged(bool) const ;
		void filesPostModChanged(bool) const ;
		void transfersChanged() const ;
		void friendsChanged() const ;
		void lobbyListChanged() const ;
        void chatLobbyEvent(qulonglong,int,const RsGxsId&,const QString&) ;
		void neighboursChanged() const ;
		void messagesChanged() const ;
		void messagesTagsChanged() const;
#ifdef REMOVE
		void forumsChanged() const ; // use connect with Qt::QueuedConnection
		void channelsChanged(int type) const ; // use connect with Qt::QueuedConnection
#endif
		void configChanged() const ;
		void logInfoChanged(const QString&) const ;
		void chatStatusChanged(const ChatId&,const QString&) const ;
		void chatCleared(const ChatId&) const ;
		void peerHasNewCustomStateString(const QString& /* peer_id */, const QString& /* status_string */) const ;
		void gotTurtleSearchResult(qulonglong search_id,FileDetail file) const ;
		void peerHasNewAvatar(const QString& peer_id) const ;
		void ownAvatarChanged() const ;
		void ownStatusMessageChanged() const ;
		void errorOccurred(int,int,const QString&) const ;
		void diskFull(int,int) const ;
		void peerStatusChanged(const QString& /* peer_id */, int /* status */);
		void peerStatusChangedSummary() const;
        void gxsChange(const RsGxsChanges& /* changes  */);
#ifdef REMOVE
		void publicChatChanged(int type) const ;
		void privateChatChanged(int list, int type) const ;
#endif
        void chatMessageReceived(ChatMessage msg);
		void groupsChanged(int type) const ;
		void discInfoChanged() const ;
		void downloadComplete(const QString& /* fileHash */);
		void downloadCompleteCountChanged(int /* count */);
#ifdef REMOVE
		void forumMsgReadSatusChanged(const QString& forumId, const QString& msgId, int status);
		void channelMsgReadSatusChanged(const QString& channelId, const QString& msgId, int status);
#endif
		void historyChanged(uint msgId, int type);
		void chatLobbyInviteReceived() ;
		void deferredSignatureHandlingRequested() ;
		void chatLobbyTimeShift(int time_shift) ;

		/* Notify from GUI */
		void chatFontChanged();
		void chatStyleChanged(int /*ChatStyle::enumStyleType*/ styleType);
		void settingsChanged();
		void disableAllChanged(bool disableAll) const;

	public slots:
		void UpdateGUI(); /* called by timer */
		void SetDisableAll(bool bValue);

	private slots:
		void runningTick();
		void handleSignatureEvent() ;
		void handleChatLobbyTimeShift(int) ;

	private:
		NotifyQt();

		static NotifyQt *_instance;
		static bool _disableAllToaster;

		/* system notifications */

		void startWaitingToasters();

//		QMutex waitingToasterMutex; // for lock of the waiting toaster list
		QList<ToasterItem*> waitingToasterList;

		QTimer *runningToasterTimer;
		QList<ToasterItem*> runningToasterList;

		bool _enabled ;
		QMutex _mutex ;

		std::map<std::string,SignatureEventData*> _deferred_signature_queue ;

		/* so we can update windows */
		NetworkDialog *cDialog;
};

#endif
