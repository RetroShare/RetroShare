#ifndef RSIFACE_NOTIFY_TXT_H
#define RSIFACE_NOTIFY_TXT_H

#include <retroshare/rsiface.h>
#include <retroshare/rsturtle.h>
#include <QObject>

#include <string>

class NetworkDialog;
class PeersDialog;
class SharedFilesDialog;
class TransfersDialog;
class ChatDialog;
class MessagesDialog;
class ChannelsDialog;
class MessengerWindow;
struct TurtleFileInfo;

//class NotifyQt: public NotifyBase, public QObject
class NotifyQt: public QObject, public NotifyBase
{
	Q_OBJECT
	public:
		static NotifyQt *Create ();
		static NotifyQt *getInstance ();

		virtual ~NotifyQt() { return; }

		void setNetworkDialog(NetworkDialog *c) { cDialog = c; }

		virtual void notifyListPreChange(int list, int type);
		virtual void notifyListChange(int list, int type);
		virtual void notifyErrorMsg(int list, int sev, std::string msg);
		virtual void notifyChatStatus(const std::string& peer_id,const std::string& status_string,bool is_private);
		virtual void notifyCustomState(const std::string& peer_id, const std::string& status_string);
		virtual void notifyHashingInfo(uint32_t type, const std::string& fileinfo);
		virtual void notifyTurtleSearchResult(uint32_t search_id,const std::list<TurtleFileInfo>& found_files);
		virtual void notifyPeerHasNewAvatar(std::string peer_id) ;
		virtual void notifyOwnAvatarChanged() ;
		virtual void notifyOwnStatusMessageChanged() ;
		virtual void notifyDiskFull(uint32_t loc,uint32_t size_in_mb) ;
		/* peer has changed the state */
		virtual void notifyPeerStatusChanged(const std::string& peer_id, uint32_t state);
		/* one or more peers has changed the states */
		virtual void notifyPeerStatusChangedSummary();
		virtual void notifyChannelMsgReadSatusChanged(const std::string& channelId, const std::string& msgId, uint32_t status);

		virtual void notifyDiscInfoChanged() ;
		virtual std::string askForPassword(const std::string& key_details,bool prev_is_bad) ;

		/* Notify from GUI */
		void notifyChatStyleChanged(int /*ChatStyle::enumStyleType*/ styleType);

	signals:
		// It's beneficial to send info to the GUI using signals, because signals are thread-safe
		// as they get queued by Qt.
		//
		void hashingInfoChanged(const QString&) const ;
		void filesPreModChanged(bool) const ;
		void filesPostModChanged(bool) const ;
		void transfersChanged() const ;
		void friendsChanged() const ;
		void neighborsChanged() const ;
		void messagesChanged() const ;
		void messagesTagsChanged() const;
		void forumsChanged() const ; // use connect with Qt::QueuedConnection
		void channelsChanged(int type) const ; // use connect with Qt::QueuedConnection
		void configChanged() const ;
		void logInfoChanged(const QString&) const ;
		void chatStatusChanged(const QString&,const QString&,bool) const ;
		void peerHasNewCustomStateString(const QString& /* peer_id */, const QString& /* status_string */) const ;
		void gotTurtleSearchResult(qulonglong search_id,FileDetail file) const ;
		void peerHasNewAvatar(const QString& peer_id) const ;
		void ownAvatarChanged() const ;
		void ownStatusMessageChanged() const ;
		void errorOccurred(int,int,const QString&) const ;
		void diskFull(int,int) const ;
		void peerStatusChanged(const QString& /* peer_id */, int /* status */);
		void peerStatusChangedSummary() const;
		void publicChatChanged(int type) const ;
		void privateChatChanged(int list, int type) const ;
		void groupsChanged(int type) const ;
		void discInfoChanged() const ;
		void channelMsgReadSatusChanged(const QString& channelId, const QString& msgId, int status);

		/* Notify from GUI */
		void chatStyleChanged(int /*ChatStyle::enumStyleType*/ styleType);

	public slots:

		void	UpdateGUI(); /* called by timer */

	private:
		NotifyQt() : cDialog(NULL) { return; }

		static NotifyQt *_instance;

//		void displayNeighbours();
//		void displayFriends();
//		void displayDirectories();
//		void displaySearch();
//		void displayChat();
//		void displayMessages();
//		void displayChannels();
//		void displayTransfers();

//		void preDisplayNeighbours();
//		void preDisplayFriends();
//		void preDisplayDirectories();
//		void preDisplaySearch();
//		void preDisplayMessages();
//		void preDisplayChannels();
//		void preDisplayTransfers();

		/* so we can update windows */
		NetworkDialog *cDialog;
//		PeersDialog       *pDialog;
//		SharedFilesDialog *dDialog;
//		TransfersDialog   *tDialog;
//		ChatDialog        *hDialog;
//		MessagesDialog    *mDialog;
//		ChannelsDialog    *sDialog;
//		MessengerWindow   *mWindow;

//		RsIface           *iface;
};

#endif
