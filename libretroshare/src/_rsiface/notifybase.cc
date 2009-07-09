#include "notifybase.h"

NotifyBase::NotifyBase()
{
}


NotifyBase::~NotifyBase()
{
}

void NotifyBase::notifyListPreChange(int list, int type)
{
    (void) list;
    (void) type;
}

void NotifyBase::notifyListChange(int list, int type)
{
    (void) list;
    (void) type;
}

void NotifyBase::notifyErrorMsg(int list, int sev, std::string msg)
{
    (void) list;
    (void) sev;
    (void) msg;
}

void NotifyBase::notifyChat()
{
}

void NotifyBase::notifyChatStatus(const std::string& peer_id,const std::string& status_string)
{
}

void NotifyBase::notifyHashingInfo(std::string fileinfo)
{
    (void)fileinfo;

}

void NotifyBase::notifyTurtleSearchResult(uint32_t search_id,const std::list<TurtleFileInfo>& files)
{
    (void)files;
}
