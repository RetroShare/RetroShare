#include "PluginNotifier.h"

void PluginNotifier::notifyReceivedVoipInvite(const std::string& peer_id)
{
	emit voipInvitationReceived(QString::fromStdString(peer_id)) ;
}
void PluginNotifier::notifyReceivedVoipData(const std::string& peer_id)
{
	emit voipDataReceived(QString::fromStdString(peer_id)) ;
}
void PluginNotifier::notifyReceivedVoipAccept(const std::string& peer_id)
{
	emit voipAcceptReceived(QString::fromStdString(peer_id)) ;
}
void PluginNotifier::notifyReceivedVoipHangUp(const std::string& peer_id)
{
	emit voipHangUpReceived(QString::fromStdString(peer_id)) ;
}
