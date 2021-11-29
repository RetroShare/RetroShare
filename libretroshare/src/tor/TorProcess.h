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

#ifndef TORPROCESS_H
#define TORPROCESS_H

#include <QObject>
#include <QHostAddress>

#include "bytearray.h"

namespace Tor
{

class TorProcessPrivate;

// This class is used to inherit calls from the TorProcess

class TorProcessClient
{
public:
    virtual void processStateChanged(int) = 0;
    virtual void processErrorChanged(const std::string&) = 0;
    virtual void processLogMessage(const std::string&) = 0;
};

/* Launches and controls a Tor instance with behavior suitable for bundling
 * an instance with the application. */
class TorProcess
{
    //Q_OBJECT
    //Q_ENUMS(State)

    //Q_PROPERTY(State state READ state NOTIFY stateChanged)
    //Q_PROPERTY(std::string errorMessage READ errorMessage NOTIFY errorMessageChanged)

public:
    enum State {
        Failed = -1,
        NotStarted,
        Starting,
        Connecting,
        Ready
    };

    explicit TorProcess(TorProcessClient *client,QObject *parent = 0);
    virtual ~TorProcess();

    std::string executable() const;
    void setExecutable(const std::string &path);

    std::string dataDir() const;
    void setDataDir(const std::string &path);

    std::string defaultTorrc() const;
    void setDefaultTorrc(const std::string &path);

    std::list<std::string> extraSettings() const;
    void setExtraSettings(const std::list<std::string> &settings);

    State state() const;
    std::string errorMessage() const;
    QHostAddress controlHost();
    quint16 controlPort();
    ByteArray controlPassword();

//signals:
    void stateChanged(int newState);
    void errorMessageChanged(const std::string &errorMessage);
    void logMessage(const std::string &message);

//public slots:
    void start();
    void stop();

private:
    TorProcessPrivate *d;
    TorProcessClient *m_client;
};

}

#endif

