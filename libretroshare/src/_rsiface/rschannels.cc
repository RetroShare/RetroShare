#include "rschannels.h"

std::ostream &operator<<(std::ostream &out, const ChannelInfo &info)
{
        std::string name(info.channelName.begin(), info.channelName.end());
        std::string desc(info.channelDesc.begin(), info.channelDesc.end());

        out << "ChannelInfo:";
        out << std::endl;
        out << "ChannelId: " << info.channelId << std::endl;
        out << "ChannelName: " << name << std::endl;
        out << "ChannelDesc: " << desc << std::endl;
        out << "ChannelFlags: " << info.channelFlags << std::endl;
        out << "Pop: " << info.pop << std::endl;
        out << "LastPost: " << info.lastPost << std::endl;

        return out;
}

std::ostream &operator<<(std::ostream &out, const ChannelMsgSummary &info)
{
        out << "ChannelMsgSummary:";
        out << std::endl;
        out << "ChannelId: " << info.channelId << std::endl;

        return out;
}

std::ostream &operator<<(std::ostream &out, const ChannelMsgInfo &info)
{
        out << "ChannelMsgInfo:";
        out << std::endl;
        out << "ChannelId: " << info.channelId << std::endl;

        return out;
}


RsChannels *rsChannels = NULL;
