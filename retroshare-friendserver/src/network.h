/*
 * RetroShare Friend Server
 * Copyright (C) 2021-2021  retroshare team <retroshare.project@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-FileCopyrightText: Retroshare Team <contact@retroshare.cc>
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#pragma once

#include "util/rsthreads.h"
#include "pqi/pqi_base.h"

class FsNetworkInterface: public BinInterface
{
public:
    FsNetworkInterface() ;

    void start() ;

    // Implements BinInterface methods

    virtual int tick() override;

    virtual int senddata(void *data, int len) override;
    virtual int readdata(void *data, int len) override;

    virtual int netstatus() override;
    virtual int isactive() override;
    virtual bool moretoread(uint32_t usec) override;
    virtual bool cansend(uint32_t usec) override;

    virtual int close() override;

    /**
     * If hashing data
     **/
    virtual RsFileHash gethash() override { return RsFileHash() ; }
    virtual uint64_t bytecount() override { return mTotalBytes; }

    virtual bool bandwidthLimited() override { return false; }
private:

    void initListening();
    void stopListening();

    int mClintListn ;
    uint64_t mTotalBytes;
};
