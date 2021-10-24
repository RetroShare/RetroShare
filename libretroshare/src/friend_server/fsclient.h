// This class runs a client connection to the friend server. It opens a socket at each connection.

class FsClient
{
public:
    FsClient(const std::string& address);

    bool sendItem(RsItem *item);

private:
    std::string mServerAddress;
};

