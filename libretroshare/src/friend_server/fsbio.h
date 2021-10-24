class FsBioInterface: public BinInterface
{
public:
    FsBioInterface(int socket);

     // Implements BinInterface methods

    int tick() override;

    int senddata(void *data, int len) override;
    int readdata(void *data, int len) override;

    int netstatus() override;
    int isactive() override;
    bool moretoread(uint32_t usec) override;
    bool cansend(uint32_t usec) override;

    int close() override;

    /**
     * If hashing data
     **/
    RsFileHash gethash() override { return RsFileHash() ; }
    uint64_t bytecount() override { return mTotalReadBytes; }

    bool bandwidthLimited() override { return false; }

private:
    int mCLintConnt;
    uint32_t mTotalReadBytes;
    uint32_t mTotalBufferBytes;

    std::list<std::pair<void *,int> > in_buffer;
};

