#ifndef NOTIFYBASE_H
#define NOTIFYBASE_H

#include "rsdefs.h"

class NotifyBase
{
public:
    NotifyBase();
    virtual ~NotifyBase();
    virtual void notifyListPreChange(int list, int type);
    virtual void notifyListChange(int list, int type);
    virtual void notifyErrorMsg(int list, int sev, std::string msg);
    virtual void notifyChat();
    virtual void notifyChatStatus(const std::string& peer_id,const std::string& status_string);
    virtual void notifyHashingInfo(std::string fileinfo);
    virtual void notifyTurtleSearchResult(uint32_t search_id,const std::list<TurtleFileInfo>& files);
};




#endif // NOTIFYBASE_H
