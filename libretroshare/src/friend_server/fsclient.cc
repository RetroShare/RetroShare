FsClient::FsClient(const std::string& address)
    : mServerAddress(address)
{
}

bool FsClient::sendItem(RsItem *item)
{
    // open a connection

    int CreateSocket = 0,n = 0;
    char dataReceived[1024];
    struct sockaddr_in ipOfServer;

    memset(dataReceived, '0' ,sizeof(dataReceived));

    if((CreateSocket = socket(AF_INET, SOCK_STREAM, 0))< 0)
    {
        printf("Socket not created \n");
        return 1;
    }

    ipOfServer.sin_family = AF_INET;
    ipOfServer.sin_port = htons(2017);
    ipOfServer.sin_addr.s_addr = inet_addr("127.0.0.1");

    if(connect(CreateSocket, (struct sockaddr *)&ipOfServer, sizeof(ipOfServer))<0)
    {
        printf("Connection failed due to port and ip problems, or server is not available\n");
        return false;
    }

    // Serialise the item and send it.

    uint32_t size = RsSerialiser::MAX_SERIAL_SIZE;
    RsTemporaryMemory data(size);

    if(!data)
    {
        RsErr() << "Cannot allocate memory to send item!" << std::endl;
        return false;
    }

    FsSerializer *fss = new FsSerializer;
    RsSerialiser rss;
    rss.addSerialType(fss);

    FsSerializer().serialise(item,data,&size);

    // TODO: we should write in multiple chunks just in case the socket is not fully ready
    write(CreateSocket,data,size);

    // Now attempt to read and deserialize anything that comes back from that connexion

    FsBioInterface bio(CreateSocket);
    pqistreamer pqi(&rss,RsPeerId(),&bio,BIN_FLAGS_READABLE);
    pqithreadstreamer p(&pqi,&rss,RsPeerId(),&bio,BIN_FLAGS_READABLE);
    p.start();

    while(true)
    {
        RsItem *item = p.GetItem();

        if(!item)
        {
            rstime::rs_usleep(1000*200);
            continue;
        }

        std::cerr << "Got a response item: " << std::endl;
        std::cerr << *item << std::endl;
    }

    return 0;

    // if ok, stream the item through it
}

