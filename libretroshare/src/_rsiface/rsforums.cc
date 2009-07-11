#include "rsforums.h"

std::ostream &operator<<(std::ostream &out, const ForumInfo &info)
{
    std::string name(info.forumName.begin(), info.forumName.end());
    std::string desc(info.forumDesc.begin(), info.forumDesc.end());

    out << "ForumInfo:";
    out << std::endl;
    out << "ForumId: " << info.forumId << std::endl;
    out << "ForumName: " << name << std::endl;
    out << "ForumDesc: " << desc << std::endl;
    out << "ForumFlags: " << info.forumFlags << std::endl;
    out << "Pop: " << info.pop << std::endl;
    out << "LastPost: " << info.lastPost << std::endl;

    return out;
}

std::ostream &operator<<(std::ostream &out, const ForumMsgInfo &info)
{
    out << "ForumMsgInfo:";
    out << std::endl;
    //out << "ForumId: " << forumId << std::endl;
    //out << "ThreadId: " << threadId << std::endl;

    return out;
}


std::ostream &operator<<(std::ostream &out, const ThreadInfoSummary &info)
{
    out << "ThreadInfoSummary:";
    out << std::endl;
    //out << "ForumId: " << forumId << std::endl;
    //out << "ThreadId: " << threadId << std::endl;

    return out;
}
