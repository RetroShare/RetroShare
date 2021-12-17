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
#include "pqi/pqifdbin.h"

RsFdBinInterface::RsFdBinInterface(int file_descriptor)
    : mCLintConnt(file_descriptor),mIsActive(false)
{
    mTotalReadBytes=0;
    mTotalInBufferBytes=0;
    mTotalWrittenBytes=0;
    mTotalOutBufferBytes=0;

    if(file_descriptor!=0)
        setSocket(file_descriptor);
}

void RsFdBinInterface::setSocket(int s)
{
        if(mIsActive != 0)
        {
            RsErr() << "Changing socket to active FsBioInterface! Canceling all pending R/W data." ;
            close();
        }
    int flags = fcntl(s,F_GETFL);

    if(!(flags & O_NONBLOCK))
        throw std::runtime_error("Trying to use a blocking file descriptor in RsFdBinInterface. This is not going to work!");

    mCLintConnt = s;
    mIsActive = (s!=0);
}
int RsFdBinInterface::tick()
{
    if(!mIsActive)
    {
        RsErr() << "Ticking a non active FsBioInterface!" ;
        return 0;
    }
    // 2 - read incoming data pending on existing connections

    int res=0;

    res += read_pending();
    res += write_pending();

    return res;
}

int RsFdBinInterface::read_pending()
{
    char inBuffer[1025];
    memset(inBuffer,0,1025);

    ssize_t readbytes = read(mCLintConnt, inBuffer, sizeof(inBuffer));	// Needs read instead of recv which is only for sockets.
                                                                        // Sockets should be set to non blocking by the client process.

    if(readbytes == 0)
    {
        RsDbg() << "Reached END of the stream!" ;
        RsDbg() << "Closing!" ;

        close();
        return mTotalInBufferBytes;
    }
    if(readbytes < 0)
    {
        if(errno != EWOULDBLOCK && errno != EAGAIN)
            RsErr() << "read() failed. Errno=" << errno ;

        return mTotalInBufferBytes;
    }

    RsDbg() << "clintConnt: " << mCLintConnt << ", readbytes: " << readbytes ;

    // display some debug info

    if(readbytes > 0)
    {
        //RsDbg() << "Received the following bytes: " << RsUtil::BinToHex( reinterpret_cast<unsigned char*>(inBuffer),readbytes,50) << std::endl;
        RsDbg() << "Received the following bytes: " << std::string(inBuffer,readbytes) << std::endl;

        void *ptr = malloc(readbytes);

        if(!ptr)
            throw std::runtime_error("Cannot allocate memory! Go buy some RAM!");

        memcpy(ptr,inBuffer,readbytes);

        in_buffer.push_back(std::make_pair(ptr,readbytes));
        mTotalInBufferBytes += readbytes;
        mTotalReadBytes += readbytes;

        RsDbg() << "Socket: " << mCLintConnt << ". Total read: " << mTotalReadBytes << ". Buffer size: " << mTotalInBufferBytes ;
    }
    return mTotalInBufferBytes;
}

int RsFdBinInterface::write_pending()
{
    if(out_buffer.empty())
        return mTotalOutBufferBytes;

    auto& p = out_buffer.front();
    int written = write(mCLintConnt, p.first, p.second);

    if(written < 0)
    {
        if(errno != EWOULDBLOCK && errno != EAGAIN)
            RsErr() << "write() failed. Errno=" << errno ;

        return mTotalOutBufferBytes;
    }

    if(written == 0)
    {
        RsErr() << "write() failed. Nothing sent.";
        return mTotalOutBufferBytes;
    }

    RsDbg() << "clintConnt: " << mCLintConnt << ", written: " << written ;

    // display some debug info

    RsDbg() << "Sent the following bytes: " << RsUtil::BinToHex( reinterpret_cast<unsigned char*>(p.first),written,50) << std::endl;

    if(written < p.second)
    {
        void *ptr = malloc(p.second - written);

        if(!ptr)
            throw std::runtime_error("Cannot allocate memory! Go buy some RAM!");

        memcpy(ptr,static_cast<unsigned char *>(p.first) + written,p.second - written);
        free(p.first);

        out_buffer.front().first = ptr;
        out_buffer.front().second = p.second - written;
    }
    else
    {
        free(p.first);
        out_buffer.pop_front();
    }

    mTotalOutBufferBytes -= written;
    mTotalWrittenBytes += written;

    return mTotalOutBufferBytes;
}

RsFdBinInterface::~RsFdBinInterface()
{
    clean();
}

void RsFdBinInterface::clean()
{
    for(auto p:in_buffer) free(p.first);
    for(auto p:out_buffer) free(p.first);

    in_buffer.clear();
    out_buffer.clear();
}

int RsFdBinInterface::readline(void *data, int len)
{
    int n=0;

    for(auto p:in_buffer)
        for(int i=0;i<p.second;++i,++n)
            if((n+1==len) || static_cast<unsigned char*>(p.first)[i] == '\n')
                return readdata(data,n+1);

    return 0;
}
int RsFdBinInterface::readdata(void *data, int len)
{
    // read incoming bytes in the buffer

    int total_len = 0;

    while(total_len < len)
    {
        if(in_buffer.empty())
        {
            mTotalInBufferBytes -= total_len;
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

            mTotalInBufferBytes -= len;
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
    mTotalInBufferBytes -= len;
    return len;
}

int RsFdBinInterface::senddata(void *data, int len)
{
    // shouldn't we better send in multiple packets, similarly to how we read?

    if(len == 0)
    {
        RsErr() << "Calling FsBioInterface::senddata() with null size or null data pointer";
        return 0;
    }
    void *ptr = malloc(len);

    if(!ptr)
    {
        RsErr() << "Cannot allocate data of size " << len ;
        return 0;
    }

    memcpy(ptr,data,len);
    out_buffer.push_back(std::make_pair(ptr,len));

    mTotalOutBufferBytes += len;
    return len;
}
int RsFdBinInterface::netstatus()
{
    return mIsActive; // dummy response.
}

int RsFdBinInterface::isactive()
{
    return mIsActive ;
}

bool RsFdBinInterface::moretoread(uint32_t /* usec */)
{
    return mTotalInBufferBytes > 0;
}
bool RsFdBinInterface::cansend(uint32_t)
{
    return isactive();
}

int RsFdBinInterface::close()
{
    RsDbg() << "Stopping network interface" << std::endl;
    mIsActive = false;
    mCLintConnt = 0;
    clean();

    return 1;
}


