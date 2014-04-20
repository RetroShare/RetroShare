#include "PluginNotifier.h"

void PluginNotifier::notifyReceivedVoipInvite(const RsPeerId& peer_id)
{
    emit voipInvitationReceived(QString::fromStdString(peer_id.toStdString())) ;
}
void PluginNotifier::notifyReceivedVoipData(const RsPeerId &peer_id)
{
    emit voipDataReceived(QString::fromStdString(peer_id.toStdString())) ;
}
void PluginNotifier::notifyReceivedVoipAccept(const RsPeerId& peer_id)
{
    emit voipAcceptReceived(QString::fromStdString(peer_id.toStdString())) ;
}
void PluginNotifier::notifyReceivedVoipHangUp(const RsPeerId &peer_id)
{
    emit voipHangUpReceived(QString::fromStdString(peer_id.toStdString())) ;
}
