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

#include "pqi/rstcpsocket.h"
#include "bytearray.h"

namespace Tor
{

class TorControlCommand;

class TorControlSocketClient
{
public:
    virtual void socketError(const std::string& s) = 0;
};

class TorControlSocket : public RsThreadedTcpSocket
{
public:
    explicit TorControlSocket(TorControlSocketClient *client);
    virtual ~TorControlSocket();

    std::string errorMessage() const { return m_errorMessage; }

    void connectToHost(const std::string& tcp_address,uint16_t tcp_port);
    void registerEvent(const ByteArray &event, TorControlCommand *handler);

    void sendCommand(const ByteArray& data) { sendCommand(0, data); }
    void sendCommand(TorControlCommand *command, const ByteArray &data);

    ByteArray readline(int s);

    // threaded TcpSocket

    virtual int tick() override;

    std::string peerAddress() const;
//signals:
//    void error(const std::string& message);

    const std::string& errorString() const { return m_errorMessage ;}

//private slots:
    void process();
    void clear();

private:
    std::list<TorControlCommand*> commandQueue;
    std::map<ByteArray,TorControlCommand*> eventCommands;
    std::string m_errorMessage;
    TorControlCommand *currentCommand;
    bool inDataReply;
    TorControlSocketClient *mClient;

    void setError(const std::string& message);
};

}
