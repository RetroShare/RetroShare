// This class receives async-ed signals from the plugin notifier,
// and executes GUI requests.
//
// It is never called directly: it is called by Qt signal-received callback,
// in the main GUI thread.
//

#include <QObject>

class PluginGUIHandler: public QObject
{
	Q_OBJECT

	public slots:
		void ReceivedInvitation(const QString& peer_id) ;
		void ReceivedVoipData(const QString& peer_id) ;
		void ReceivedVoipHangUp(const QString& peer_id) ;
		void ReceivedVoipAccept(const QString& peer_id) ;
};
