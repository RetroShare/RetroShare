#pragma once
#include "StateTokenServer.h"

namespace resource_api{

// store for temporary binary data
// use cases: upload of avatar image, upload of certificate file
// those files are first store in this class, then and a handler will fetch the datas later
// blobs are removed if they are not used after xx hours
class TmpBlobStore: private Tickable
{
public:
    explicit TmpBlobStore(StateTokenServer* sts);
    virtual ~TmpBlobStore();

    // from Tickable
    virtual void tick();

    // 30MB should be enough for avatars pictures, pdfs and mp3s
    // can remove this limit, once we can store data on disk
    static const int MAX_BLOBSIZE = 30*1000*1000;
    static const int BLOB_STORE_TIME_SECS = 12*60*60;

    // steals the bytes
    // returns a blob number as identifier
    // returns null on failure
    uint32_t storeBlob(std::vector<uint8_t>& bytes);
    // fetch blob with given id
    // the blob is removed from the store
    // return true on success
    bool fetchBlob(uint32_t blobid, std::vector<uint8_t>& bytes);

private:
    class Blob{
    public:
        std::vector<uint8_t> bytes;
        time_t timestamp;
        uint32_t id;
        Blob* next;
    };

    Blob* locked_findblob(uint32_t id);
    uint32_t locked_make_id();

    StateTokenServer* const mStateTokenServer;

    RsMutex mMtx;

    Blob* mBlobs;

};

} // namespace resource_api
