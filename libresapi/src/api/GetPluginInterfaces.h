#pragma once

class RsPlugInInterfaces;

namespace resource_api{

// populates the given RsPlugInInterfaces object with pointers from gloabl variables like rsPeers, rsMsgs, rsFiles...
bool getPluginInterfaces(RsPlugInInterfaces& interfaces);

} // namespace resource_api
