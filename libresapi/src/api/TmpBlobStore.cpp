/*******************************************************************************
 * libresapi/api/TmpBLogStore.cpp                                              *
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
#include "TmpBlobStore.h"

#include <util/rsrandom.h>
#include <time.h>

namespace resource_api{

TmpBlobStore::TmpBlobStore(StateTokenServer* sts):
    mStateTokenServer(sts), mMtx("TmpBlobStore::mMtx"), mBlobs(0)
{
    mStateTokenServer->registerTickClient(this);
}
TmpBlobStore::~TmpBlobStore()
{
    mStateTokenServer->unregisterTickClient(this);

    RS_STACK_MUTEX(mMtx);
    Blob* blob = mBlobs;
    while(blob)
    {
        Blob* next = blob->next;
        delete blob;
        blob = next;
    }
}

void TmpBlobStore::tick()
{
    RS_STACK_MUTEX(mMtx);
    Blob* prev = 0;
    Blob* current = mBlobs;

    time_t now = time(NULL);

    for(; current != 0; current = current->next)
    {
        if(current->timestamp + BLOB_STORE_TIME_SECS < now)
        {
            if(prev)
                prev->next = current->next;
            else
                mBlobs = current->next;
            delete current;
            return;
        }
        prev = current;
    }
}

uint32_t TmpBlobStore::storeBlob(std::vector<uint8_t>& bytes)
{
    if(bytes.size() > MAX_BLOBSIZE)
        return 0;

    RS_STACK_MUTEX(mMtx);

    Blob* blob = new Blob();
    blob->bytes.swap(bytes);
    blob->timestamp = time(NULL);
    blob->id = locked_make_id();
    blob->next = 0;

    if(!mBlobs)
    {
        mBlobs = blob;
        return blob->id;
    }

    Blob* blob2 = mBlobs;
    while(blob2->next)
        blob2 = blob2->next;

    blob2->next = blob;
    return blob->id;
}

bool TmpBlobStore::fetchBlob(uint32_t blobid, std::vector<uint8_t>& bytes)
{
    RS_STACK_MUTEX(mMtx);
    Blob* prev = 0;
    Blob* current = mBlobs;

    for(; current != 0; current = current->next)
    {
        if(current->id == blobid)
        {
            bytes.swap(current->bytes);
            if(prev)
                prev->next = current->next;
            else
                mBlobs = current->next;
            delete current;
            return true;
        }
        prev = current;
    }
    return false;
}

TmpBlobStore::Blob* TmpBlobStore::locked_findblob(uint32_t id)
{
    Blob* blob;
    for(blob = mBlobs; blob != 0; blob = blob->next)
    {
        if(blob->id == id)
            return blob;
    }
    return 0;
}

uint32_t TmpBlobStore::locked_make_id()
{
    uint32_t id = RSRandom::random_u32();
    // make sure the id is not in use already
    while(locked_findblob(id))
        id = RSRandom::random_u32();
    return id;
}

} // namespace resource_api
