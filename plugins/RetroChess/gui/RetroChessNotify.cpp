#include "RetroChessNotify.h"

RetroChessNotify::RetroChessNotify(QObject *parent) : QObject(parent)
{

}

void RetroChessNotify::notifyReceivedPaint(const RsPeerId &peer_id, int x, int y)
{
	std::cout << "pNotify Recvd paint from: " << peer_id;
	std::cout << " at " << x << " , " << y;
	std::cout << std::endl;
}


void RetroChessNotify::notifyReceivedMsg(const RsPeerId& peer_id, QString str)
{
	std::cout << "pNotify Recvd Packet from: " << peer_id;
	std::cout << " saying " << str.toStdString();
	std::cout << std::endl;
	emit NeMsgArrived(peer_id, str) ;
}

void RetroChessNotify::notifyChessStart(const RsPeerId &peer_id)
{
	emit chessStart(peer_id) ;

}
void RetroChessNotify::notifyChessInvite(const RsPeerId &peer_id)
{
	emit chessInvited(peer_id) ;

}
