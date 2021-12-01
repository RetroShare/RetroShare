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

#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>

#include "util/rsdir.h"

#include "TorProcess_p.h"
#include "CryptoKey.h"
#include "SecureRNG.h"
#include <QDir>
#include <QDebug>
#include <QCoreApplication>

using namespace Tor;

TorProcess::TorProcess(TorProcessClient *client,QObject *parent)
    : d(new TorProcessPrivate(this)),m_client(client)
{
}

TorProcess::~TorProcess()
{
    if (state() > NotStarted)
        stop();
}

TorProcessPrivate::TorProcessPrivate(TorProcess *q)
    : q(q), state(TorProcess::NotStarted), controlPort(0), controlPortAttempts(0)
{
    connect(&process, &QProcess::started, this, &TorProcessPrivate::processStarted);
    connect(&process, (void (QProcess::*)(int, QProcess::ExitStatus))&QProcess::finished,
            this, &TorProcessPrivate::processFinished);
    connect(&process, (void (QProcess::*)(QProcess::ProcessError))&QProcess::error,
            this, &TorProcessPrivate::processError);
    connect(&process, &QProcess::readyRead, this, &TorProcessPrivate::processReadable);

    controlPortTimer.setInterval(500);
    connect(&controlPortTimer, &QTimer::timeout, this, &TorProcessPrivate::tryReadControlPort);
}

std::string TorProcess::executable() const
{
    return d->executable;
}

void TorProcess::setExecutable(const std::string &path)
{
    d->executable = path;
}

std::string TorProcess::dataDir() const
{
    return d->dataDir;
}

void TorProcess::setDataDir(const std::string &path)
{
    d->dataDir = path;
}

std::string TorProcess::defaultTorrc() const
{
    return d->defaultTorrc;
}

void TorProcess::setDefaultTorrc(const std::string &path)
{
    d->defaultTorrc = path;
}

std::list<std::string> TorProcess::extraSettings() const
{
    return d->extraSettings;
}

void TorProcess::setExtraSettings(const std::list<std::string> &settings)
{
    d->extraSettings = settings;
}

TorProcess::State TorProcess::state() const
{
    return d->state;
}

std::string TorProcess::errorMessage() const
{
    return d->errorMessage;
}

// Does a fopen, but dup all file descriptors (STDIN STDOUT and STDERR) to the
// FDs supplied by the parent process

int popen3(int fd[3],const char **const cmd)
{
    int i, e;
    int p[3][2];
    pid_t pid;
    // set all the FDs to invalid
    for(i=0; i<3; i++)
        p[i][0] = p[i][1] = -1;
    // create the pipes
    for(int i=0; i<3; i++)
        if(pipe(p[i]))
            goto error;
    // and fork
    pid = fork();
    if(-1 == pid)
        goto error;
    // in the parent?
    if(pid) {
        // parent
        fd[STDIN_FILENO] = p[STDIN_FILENO][1];
        close(p[STDIN_FILENO][0]);
        fd[STDOUT_FILENO] = p[STDOUT_FILENO][0];
        close(p[STDOUT_FILENO][1]);
        fd[STDERR_FILENO] = p[STDERR_FILENO][0];
        close(p[STDERR_FILENO][1]);
        // success
        return 0;
    } else {
        // child
        dup2(p[STDIN_FILENO][0],STDIN_FILENO);
        close(p[STDIN_FILENO][1]);
        dup2(p[STDOUT_FILENO][1],STDOUT_FILENO);
        close(p[STDOUT_FILENO][0]);
        dup2(p[STDERR_FILENO][1],STDERR_FILENO);
        close(p[STDERR_FILENO][0]);
        // here we try and run it
        execv(*cmd,const_cast<char*const*>(cmd));
        // if we are there, then we failed to launch our program
        perror("Could not launch");
        fprintf(stderr," \"%s\"\n",*cmd);
        _exit(EXIT_FAILURE);
    }

error:
    // preserve original error
    e = errno;
    for(i=0; i<3; i++) {
        close(p[i][0]);
        close(p[i][1]);
    }
    errno = e;
    return -1;
}

void TorProcess::start()
{
    if (state() > NotStarted)
        return;

    d->errorMessage.clear();

    if (d->executable.empty() || d->dataDir.empty()) {
        d->errorMessage = "Tor executable and data directory not specified";
        d->state = Failed;

        if(m_client) m_client->processStateChanged(d->state); // emit stateChanged(d->state);
        if(m_client) m_client->processErrorChanged(d->errorMessage); // emit errorMessageChanged(d->errorMessage);
        return;
    }

    if (!d->ensureFilesExist()) {
        d->state = Failed;
        if(m_client) m_client->processErrorChanged(d->errorMessage);// emit errorMessageChanged(d->errorMessage);
        if(m_client) m_client->processStateChanged(d->state);// emit stateChanged(d->state);
        return;
    }

    ByteArray password = controlPassword();
    ByteArray hashedPassword = torControlHashedPassword(password);
    if (password.empty() || hashedPassword.empty()) {
        d->errorMessage = "Random password generation failed";
        d->state = Failed;
        if(m_client) m_client->processErrorChanged(d->errorMessage);// emit errorMessageChanged(d->errorMessage);
        if(m_client) m_client->processStateChanged(d->state); // emit stateChanged(d->state);
    }

    std::list<std::string> args;
    if (!d->defaultTorrc.empty())
    {
        args.push_back("--defaults-torrc");
        args.push_back(d->defaultTorrc);
    }

    args.push_back("-f");
    args.push_back(d->torrcPath());

    args.push_back("DataDirectory") ;
    args.push_back(d->dataDir);

    args.push_back("HashedControlPassword") ;
    args.push_back(hashedPassword.toString());

    args.push_back("ControlPort") ;
    args.push_back("auto");

    args.push_back("ControlPortWriteToFile");
    args.push_back(d->controlPortFilePath());

    args.push_back("__OwningControllerProcess") ;
    args.push_back(RsUtil::NumberToString(getpid()));

    for(auto s:d->extraSettings)
        args.push_back(s);

    d->state = Starting;

    if(m_client) m_client->processStateChanged(d->state);// emit stateChanged(d->state);

    if (RsDirUtil::fileExists(d->controlPortFilePath()))
        RsDirUtil::removeFile(d->controlPortFilePath());

    d->controlPort = 0;
    d->controlHost.clear();

    d->process.setProcessChannelMode(QProcess::MergedChannels);
    d->process.start(d->executable, args, QIODevice::ReadOnly);
}

void TorProcess::stop()
{
    if (state() < Starting)
        return;

    d->controlPortTimer.stop();

    if (d->process.state() == QProcess::Starting)
        d->process.waitForStarted(2000);

    d->state = NotStarted;

    // Windows can't terminate the process well, but Tor will clean itself up
#ifndef Q_OS_WIN
    if (d->process.state() == QProcess::Running) {
        d->process.terminate();
        if (!d->process.waitForFinished(5000)) {
            qWarning() << "Tor process" << d->process.pid() << "did not respond to terminate, killing...";
            d->process.kill();
            if (!d->process.waitForFinished(2000)) {
                qCritical() << "Tor process" << d->process.pid() << "did not respond to kill!";
            }
        }
    }
#endif

    if(m_client) m_client->processStateChanged(d->state);// emit stateChanged(d->state);
}

void TorProcess::stateChanged(int newState)
{
    if(m_client)
        m_client->processStateChanged(newState);
}
void TorProcess::errorMessageChanged(const std::string& errorMessage)
{
    if(m_client)
        m_client->processErrorChanged(errorMessage);
}
void TorProcess::logMessage(const std::string& message)
{
    if(m_client)
        m_client->processLogMessage(message);
}

ByteArray TorProcess::controlPassword()
{
    if (d->controlPassword.empty())
        d->controlPassword = RsRandom::printable(16);

    return d->controlPassword;
}

QHostAddress TorProcess::controlHost()
{
    return d->controlHost;
}

quint16 TorProcess::controlPort()
{
    return d->controlPort;
}

bool TorProcessPrivate::ensureFilesExist()
{
    if(!RsDirUtil::checkCreateDirectory(dataDir))
    {
        errorMessage = "Cannot create Tor data directory: " + dataDir;
        return false;
    }

    if (!RsDirUtil::fileExists(torrcPath()))
    {
        FILE *f = RsDirUtil::rs_fopen(torrcPath().c_str(),"w");

        if(!f)
        {
            errorMessage = "Cannot create Tor configuration file: " + torrcPath();
            return false;
        }
        else
            fclose(f);
    }

    return true;
}

std::string TorProcessPrivate::torrcPath() const
{
    //return QDir::toNativeSeparators(dataDir) + QDir::separator() + QStringLiteral("torrc");
    return dataDir + "/" + "torrc";
}

std::string TorProcessPrivate::controlPortFilePath() const
{
    //return QDir::toNativeSeparators(dataDir) + QDir::separator() + QStringLiteral("control-port");
    return dataDir + "/" + "control-port";
}

void TorProcessPrivate::processStarted()
{
    state = TorProcess::Connecting;

    /*emit*/ q->stateChanged(state);
    /*emit*/ q->stateChanged(state);

    controlPortAttempts = 0;
    controlPortTimer.start();
}

void TorProcessPrivate::processFinished()
{
    if (state < TorProcess::Starting)
        return;

    controlPortTimer.stop();
    errorMessage = process.errorString().toStdString();

    if (errorMessage.empty())
        errorMessage = "Process exited unexpectedly (code " + RsUtil::NumberToString(process.exitCode()) + ")";

    state = TorProcess::Failed;
    /*emit*/ q->errorMessageChanged(errorMessage);
    /*emit*/ q->stateChanged(state);
}

void TorProcessPrivate::processError(QProcess::ProcessError error)
{
    if (error == QProcess::FailedToStart || error == QProcess::Crashed)
        processFinished();
}

void TorProcessPrivate::processReadable()
{
    while (process.bytesAvailable() > 0)
    {
        ByteArray line = process.readLine(2048).trimmed();

        if (!line.empty())
            /*emit*/ q->logMessage(line.toString()));
    }
}

void TorProcessPrivate::tryReadControlPort()
{
    FILE *file = RsDirUtil::rs_fopen(controlPortFilePath().c_str(),"r");

    if(file)
    {
        char *line = nullptr;

        size_t size = getline(&line,0,file);
        ByteArray data = ByteArray((unsigned char*)line,size).trimmed();
        free(line);

        int p;
        if (data.startsWith("PORT=") && (p = data.lastIndexOf(':')) > 0) {
            controlHost = QHostAddress(data.mid(5, p - 5));
            controlPort = data.mid(p+1).toUShort();

            if (!controlHost.isNull() && controlPort > 0) {
                controlPortTimer.stop();
                state = TorProcess::Ready;
                /*emit*/ q->stateChanged(state);
                return;
            }
        }
    }

    if (++controlPortAttempts * controlPortTimer.interval() > 10000) {
        errorMessage = "No control port available after launching process";
        state = TorProcess::Failed;
        /*emit*/ q->errorMessageChanged(errorMessage);
        /*emit*/ q->stateChanged(state);
    }
}

void TorProcess::run()
{
    //
}
