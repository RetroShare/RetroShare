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

