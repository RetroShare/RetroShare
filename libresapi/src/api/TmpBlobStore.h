/*******************************************************************************
 * libresapi/api/TmpBLogStore.h                                                *
 *                                                                             *
 * LibResAPI: API for local socket server                                      *
 *                                                                             *
 * Copyright 2018 by Retroshare Team <retroshare.project@gmail.com>            *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Affero General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/
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
    TmpBlobStore(StateTokenServer* sts);
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
