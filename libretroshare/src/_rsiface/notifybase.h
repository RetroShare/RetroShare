#ifndef NOTIFYBASE_H
#define NOTIFYBASE_H


const int NOTIFY_LIST_NEIGHBOURS   = 1;
const int NOTIFY_LIST_FRIENDS      = 2;
const int NOTIFY_LIST_DIRLIST      = 3;
const int NOTIFY_LIST_SEARCHLIST   = 4;
const int NOTIFY_LIST_MESSAGELIST  = 5;
const int NOTIFY_LIST_CHANNELLIST  = 6;
const int NOTIFY_LIST_TRANSFERLIST = 7;
const int NOTIFY_LIST_CONFIG       = 8;

const int NOTIFY_TYPE_SAME   = 0x01;
const int NOTIFY_TYPE_MOD    = 0x02; /* general purpose, check all */
const int NOTIFY_TYPE_ADD    = 0x04; /* flagged additions */
const int NOTIFY_TYPE_DEL    = 0x08; /* flagged deletions */

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
