#include <iostream>
#include "PluginGUIHandler.h"

void PluginGUIHandler::ReceivedInvitation(const QString& peer_id) 
{
	std::cerr << "****** Plugin GUI handler: received data!" << std::endl;
}

void PluginGUIHandler::ReceivedVoipData(const QString& peer_id) 
{
	std::cerr << "****** Plugin GUI handler: received invitation!" << std::endl;
}

