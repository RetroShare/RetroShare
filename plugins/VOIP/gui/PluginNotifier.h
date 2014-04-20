// This class is a Qt object to get notification from the plugin's service threads,
// and responsible to pass the info the the GUI part.
//
// Because the GUI part is async-ed with the service, it is crucial to use the
// QObject connect system to communicate between the p3Service and the gui part (handled by Qt)
//

#include <QObject>
#include <retroshare/rstypes.h>

class PluginNotifier: public QObject
{
	Q_OBJECT

	public:
        void notifyReceivedVoipData(const RsPeerId& peer_id) ;
        void notifyReceivedVoipInvite(const RsPeerId &peer_id) ;
        void notifyReceivedVoipHangUp(const RsPeerId& peer_id) ;
        void notifyReceivedVoipAccept(const RsPeerId &peer_id) ;

	signals:
		void voipInvitationReceived(const QString&) ;	// signal emitted when an invitation has been received
		void voipDataReceived(const QString&) ;			// signal emitted when some voip data has been received
		void voipHangUpReceived(const QString& peer_id) ; // emitted when the peer closes the call (i.e. hangs up)
		void voipAcceptReceived(const QString& peer_id) ; // emitted when the peer accepts the call
};
