/*******************************************************************************
 * libretroshare/src/file_sharing: fsbio.h                                     *
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

#include "pqi/pqi_base.h"

class RsFdBinInterface: public BinInterface
{
public:
    RsFdBinInterface(int file_descriptor, bool is_socket);
    ~RsFdBinInterface();

     // Implements BinInterface methods

    int tick() override;

    // Schedule data to be sent at the next tick(). The caller keeps memory ownership.
    //
    int senddata(void *data, int len) override;

    // Obtains new data from the interface. "data" needs to be initialized for room
    // to len bytes. The returned value is the actual size of what was read.
    //
    int readdata(void *data, int len) override;

    // Read at most len bytes only if \n is encountered within that range. Otherwise, nothing is changed.
    //
    int readline(void *data, int len) ;

    int netstatus() override;
    int isactive() override;
    bool moretoread(uint32_t usec) override;
    bool moretowrite(uint32_t usec) ;
    bool cansend(uint32_t usec) override;

    int close() override;

    /**
     * If hashing data
     **/
    RsFileHash gethash() override { return RsFileHash() ; }
    uint64_t bytecount() override { return mTotalReadBytes; }

    bool bandwidthLimited() override { return false; }

protected:
    void setSocket(int s);
    void clean();

private:
    int read_pending();
    int write_pending();

    int mCLintConnt;
    bool mIsSocket;
    bool mIsActive;
    uint32_t mTotalReadBytes;
    uint32_t mTotalInBufferBytes;
    uint32_t mTotalWrittenBytes;
    uint32_t mTotalOutBufferBytes;

    std::list<std::pair<void *,int> > in_buffer;
    std::list<std::pair<void *,int> > out_buffer;
};

