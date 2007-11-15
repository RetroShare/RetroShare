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

	public slots:

void	UpdateGUI(); /* called by timer */

	private:

	void displayNeighbours();
	void displayFriends();
	void displayDirectories();
	void displaySearch();
	void displayChat();
	void displayMessages();
	void displayChannels();
	void displayTransfers();

	void preDisplayNeighbours();
	void preDisplayFriends();
	void preDisplayDirectories();
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
