#ifndef RSIFACE_NOTIFY_TXT_H
#define RSIFACE_NOTIFY_TXT_H

#include "rsiface/rsiface.h"
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
#ifdef TURTLE_HOPPING
#include "rsiface/rsturtle.h"
class TurtleSearchDialog ;

struct TurtleFileInfo ;
#endif

//class NotifyQt: public NotifyBase, public QObject
class NotifyQt: public QObject, public NotifyBase
{
	Q_OBJECT
	public:
		NotifyQt() : cDialog(NULL), pDialog(NULL),
		dDialog(NULL), tDialog(NULL),
		hDialog(NULL), mDialog(NULL),
		sDialog(NULL), mWindow(NULL)
	{ return; }

		virtual ~NotifyQt() { return; }

		void setNetworkDialog(NetworkDialog *c) { cDialog = c; }
		void setPeersDialog(PeersDialog *p) { pDialog = p; }
		void setDirDialog(SharedFilesDialog *d) { dDialog = d; }
		void setTransfersDialog(TransfersDialog *t) { tDialog = t; }
		void setChatDialog(ChatDialog *m)         { hDialog = m; }
		void setMessagesDialog(MessagesDialog *m) { mDialog = m; }
		void setChannelsDialog(ChannelsDialog *s) { sDialog = s; }
		void setMessengerWindow(MessengerWindow *mw) { mWindow = mw; }

		void setRsIface(RsIface *i) { iface = i; }

		virtual void notifyListPreChange(int list, int type);
		virtual void notifyListChange(int list, int type);
		virtual void notifyErrorMsg(int list, int sev, std::string msg);
		virtual void notifyChat();
		virtual void notifyHashingInfo(std::string fileinfo);
#ifdef TURTLE_HOPPING
		virtual void notifyTurtleSearchResult(uint32_t search_id,const std::list<TurtleFileInfo>& found_files);
#endif

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
		void configChanged() const ;
		void gotTurtleSearchResult(qulonglong search_id,TurtleFileInfo file) const ;

	public slots:

		void	UpdateGUI(); /* called by timer */

	private:

		void displayNeighbours();
		void displayFriends();
//		void displayDirectories();
		void displaySearch();
		void displayChat();
		void displayMessages();
		void displayChannels();
		void displayTransfers();

		void preDisplayNeighbours();
		void preDisplayFriends();
//		void preDisplayDirectories();
		void preDisplaySearch();
		void preDisplayMessages();
		void preDisplayChannels();
		void preDisplayTransfers();

		/* so we can update windows */
		NetworkDialog *cDialog;
		PeersDialog       *pDialog;
		SharedFilesDialog *dDialog;
		TransfersDialog   *tDialog;
		ChatDialog        *hDialog;
		MessagesDialog    *mDialog;
		ChannelsDialog    *sDialog;
		MessengerWindow   *mWindow;

		RsIface           *iface;
};

#endif
