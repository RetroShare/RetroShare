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

#include <iostream>

#include "TorControlSocket.h"
#include "TorControlCommand.h"

using namespace Tor;

TorControlSocket::TorControlSocket(TorControlSocketClient *client)
    : RsThreadedTcpSocket(),currentCommand(0), inDataReply(false),mClient(client)
{
}

TorControlSocket::~TorControlSocket()
{
    clear();
}

bool TorControlSocket::connectToHost(const std::string& tcp_address,uint16_t tcp_port)
{
    if(RsTcpSocket::connect(tcp_address,tcp_port))
    {
        start("TorControlSocket");
        return true;
    }
    else
        return false;

}
std::string TorControlSocket::peerAddress() const
{
    if(connectionState() == State::CONNECTED)
        return connectAddress();
    else
        return std::string();
}
void TorControlSocket::sendCommand(TorControlCommand *command, const ByteArray& data)
{
    assert(data.endsWith(ByteArray("\r\n")));

    commandQueue.push_back(command);
    senddata((void*)data.data(),data.size());

    std::cerr << "[TOR CTRL] Sent: \"" << data.trimmed().toString() << "\"" << std::endl;
}

void TorControlSocket::registerEvent(const ByteArray &event, TorControlCommand *command)
{
    eventCommands.insert(std::make_pair(event, command));

    ByteArray data("SETEVENTS");
    for(auto it:eventCommands)
    {
        data += ' ';
        data += it.first;
    }
    data += "\r\n";

    sendCommand(data);
}

void TorControlSocket::clear()
{
    for(auto cmd:commandQueue) delete cmd;
    commandQueue.clear();

    for(auto cmd:eventCommands) delete cmd.second;
    eventCommands.clear();

    inDataReply = false;
    currentCommand = 0;
}

void TorControlSocket::setError(const std::string &message)
{
    m_errorMessage = message;
    mClient->socketError(message);
    abort();
}

ByteArray TorControlSocket::readline(int s)
{
    ByteArray b(s);
    int real_size;

    if(! (real_size = RsTcpSocket::readline(b.data(),s)))
        return ByteArray();
    else
    {
        b.resize(real_size);
        return b;
    }
}

void TorControlSocket::process()
{
    for (;;) {
        if (!moretoread(0))
            return;

        ByteArray line = readline(5120);

        if(line.empty())	// This happens when the incoming buffer isn't empty yet doesn't have a full line already.
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }

        if (!line.endsWith(ByteArray("\r\n"))) {
            setError("Invalid control message syntax");
            return;
        }
        line.chop(2);

        if (inDataReply) {
            if (line == ".") {
                inDataReply = false;
                if (currentCommand)
                    currentCommand->onDataFinished();
                currentCommand = 0;
            } else {
                if (currentCommand)
                    currentCommand->onDataLine(line);
            }
            continue;
        }

        if (line.size() < 4) {
            setError("Invalid control message syntax");
            return;
        }

        int statusCode = line.left(3).toInt();
        char type = line[3];
        bool isFinalReply = (type == ' ');
        inDataReply = (type == '+');

        // Trim down to just data
        line = line.mid(4);

        if (!isFinalReply && !inDataReply && type != '-') {
            setError("Invalid control message syntax");
            return;
        }

        // 6xx replies are asynchronous responses
        if (statusCode >= 600 && statusCode < 700) {
            if (!currentCommand) {
                int space = line.indexOf(' ');
                if (space > 0)
                {
                    auto it = eventCommands.find(line.mid(0, space).toString());

                    if(it != eventCommands.end())
                        currentCommand = it->second;
                }

                if (!currentCommand) {
                    RsWarn() << "torctrl: Ignoring unknown event";
                    continue;
                }
            }

            currentCommand->onReply(statusCode, line);
            if (isFinalReply) {
                currentCommand->onFinished(statusCode);
                currentCommand = 0;
            }
            continue;
        }

        if (commandQueue.empty()) {
            RsWarn() << "torctrl: Received unexpected data";
            continue;
        }

        TorControlCommand *command = commandQueue.front();
        if (command)
            command->onReply(statusCode, line);

        if (inDataReply) {
            currentCommand = command;
        } else if (isFinalReply) {
            commandQueue.pop_front();
            if (command) {
                command->onFinished(statusCode);
                delete command;		// should we "delete later" ?
            }
        }
    }
}

int TorControlSocket::tick()
{
    bool rw = RsTcpSocket::tick();

    if(moretoread(0))
        process();

    if(!rw)
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));	// temporisation when nothing happens

    return 0;	// not sure about what we should return here.
}
