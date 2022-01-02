/* Ricochet - https://ricochet.im/
 * Copyright (C) 2014, John Brooks <john.brooks@dereferenced.net>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following disclaimer
 *      in the documentation and/or other materials provided with the
 *      distribution.
 *
 *    * Neither the names of the copyright owners nor the names of its
 *      contributors may be used to endorse or promote products derived from
 *      this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include <functional>
#include "bytearray.h"

namespace Tor
{

class ProtocolInfoCommand;

class TorControlCommand
{
    friend class TorControlSocket;

public:
    TorControlCommand();
    virtual ~TorControlCommand()=default;

    int statusCode() const { return m_finalStatus; }

    void set_replyLine_callback( const std::function<void(int statusCode, const ByteArray &data)>& f) { mReplyLine=f ; }
    void set_finished_callback( const std::function<void(TorControlCommand *sender)>& f) { mFinished=f; };

public:
    virtual void onReply(int statusCode, const ByteArray &data);
    virtual void onFinished(int statusCode);
    virtual void onDataLine(const ByteArray &data);
    virtual void onDataFinished();

private:
    int m_finalStatus;

    // Disable copy
    TorControlCommand(const TorControlCommand&){}
    TorControlCommand& operator=(const TorControlCommand&){ return *this; }

    std::function<void(int statusCode, const ByteArray &data)> mReplyLine;
    std::function<void(TorControlCommand *sender)> mFinished;
};

}
