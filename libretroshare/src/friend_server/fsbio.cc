/*******************************************************************************
 * libretroshare/src/file_sharing: fsbio.cc                                    *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2021 by retroshare team <retroshare.project@gmail.com>            *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Lesser General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Lesser General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Lesser General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 ******************************************************************************/

#include "util/rsprint.h"
#include "fsbio.h"

FsBioInterface::FsBioInterface(int socket)
    : mCLintConnt(socket),mIsActive(true)
{
    mTotalReadBytes=0;
    mTotalBufferBytes=0;
}

int FsBioInterface::tick()
{
    std::cerr << "ticking FsNetworkInterface" << std::endl;

    // 2 - read incoming data pending on existing connections

    char inBuffer[1025];
    memset(inBuffer,0,1025);

    int readbytes = read(mCLintConnt, inBuffer, sizeof(inBuffer));

    if(readbytes == 0)
    {
        RsDbg() << "Reached END of the stream!" << std::endl;
        RsDbg() << "Closing!" << std::endl;

        mIsActive = false;
        return mTotalBufferBytes;
    }
    if(readbytes < 0)
    {
        if(errno != EWOULDBLOCK && errno != EAGAIN)
            RsErr() << "read() failed. Errno=" << errno ;

        return mTotalBufferBytes;
    }

    std::cerr << "clintConnt: " << mCLintConnt << ", readbytes: " << readbytes << std::endl;

    // display some debug info

    if(readbytes > 0)
    {
        RsDbg() << "Received the following bytes: " << RsUtil::BinToHex( reinterpret_cast<unsigned char*>(inBuffer),readbytes,50) << std::endl;
        //RsDbg() << "Received the following bytes: " << std::string(inBuffer,readbytes) << std::endl;

        void *ptr = malloc(readbytes);

        if(!ptr)
            throw std::runtime_error("Cannot allocate memory! Go buy some RAM!");

        memcpy(ptr,inBuffer,readbytes);

        in_buffer.push_back(std::make_pair(ptr,readbytes));
        mTotalBufferBytes += readbytes;
        mTotalReadBytes += readbytes;

        std::cerr << "Socket: " << mCLintConnt << ". Total read: " << mTotalReadBytes << ". Buffer size: " << mTotalBufferBytes << std::endl ;
    }

    return mTotalBufferBytes;
}

int FsBioInterface::readdata(void *data, int len)
{
    // read incoming bytes in the buffer

    int total_len = 0;

    while(total_len < len)
    {
        if(in_buffer.empty())
        {
            mTotalBufferBytes -= total_len;
            return total_len;
        }

        // If the remaining buffer is too large, chop of the beginning of it.

        if(total_len + in_buffer.front().second > len)
        {
            memcpy(&(static_cast<unsigned char *>(data)[total_len]),in_buffer.front().first,len - total_len);

            void *ptr = malloc(in_buffer.front().second - (len - total_len));
            memcpy(ptr,&(static_cast<unsigned char*>(in_buffer.front().first)[len - total_len]),in_buffer.front().second - (len - total_len));

            free(in_buffer.front().first);
            in_buffer.front().first = ptr;
            in_buffer.front().second -= len-total_len;

            mTotalBufferBytes -= len;
            return len;
        }
        else // copy everything
        {
            memcpy(&(static_cast<unsigned char *>(data)[total_len]),in_buffer.front().first,in_buffer.front().second);

            total_len += in_buffer.front().second;

            free(in_buffer.front().first);
            in_buffer.pop_front();
        }
    }
    mTotalBufferBytes -= len;
    return len;
}

int FsBioInterface::senddata(void *data, int len)
{
//    int written = write(mCLintConnt, data, len);
//    return written;
    return len;
}
int FsBioInterface::netstatus()
{
    return mIsActive; // dummy response.
}

int FsBioInterface::isactive()
{
    return mCLintConnt > 0;
}

bool FsBioInterface::moretoread(uint32_t /* usec */)
{
    return mTotalBufferBytes > 0;
}
bool FsBioInterface::cansend(uint32_t)
{
    return isactive();
}

int FsBioInterface::close()
{
    RsDbg() << "Stopping network interface" << std::endl;
    mIsActive = false;
    return 1;
}


