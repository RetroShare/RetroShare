#include <QTimer>
#include "PopupDistantChatDialog.h"

PopupDistantChatDialog::PopupDistantChatDialog(QWidget *parent, Qt::WFlags flags)
	: PopupChatDialog(parent,flags)
{
	_tunnel_check_timer = new QTimer;
	_tunnel_check_timer->setInterval(1000) ;

	QObject::connect(_tunnel_check_timer,SIGNAL(timeout()),this,SLOT(checkTunnel())) ;

	_tunnel_check_timer->start() ;
}

void PopupDistantChatDialog::init(const std::string& hash,const QString & title)
{
	_hash = hash ;
}

void PopupDistantChatDialog::checkTunnel()
{
	if(!isVisible())
		return ;

	std::cerr << "Checking tunnel..." ;
	// make sure about the tunnel status
	//
	
	uint32_t status = rsMsgs->getDistantChatStatus(_hash) ;

	switch(status)
	{
		case RS_DISTANT_CHAT_STATUS_UNKNOWN: std::cerr << "Unknown hash. Error!" << std::endl;
														 break ;
		case RS_DISTANT_CHAT_STATUS_TUNNEL_DN: std::cerr << "Tunnel asked. Waiting for reponse. " << std::endl;
														 break ;
		case RS_DISTANT_CHAT_STATUS_TUNNEL_OK: std::cerr << "Tunnel is ok. " << std::endl;
														 break ;
		case RS_DISTANT_CHAT_STATUS_CAN_TALK: std::cerr << "Tunnel is ok and works. You can talk!" << std::endl;
														 break ;
	}
}










