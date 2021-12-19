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
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>

#include "util/rsdir.h"
#include "pqi/pqifdbin.h"

#include "TorProcess.h"
#include "CryptoKey.h"

using namespace Tor;

static const int INTERVAL_BETWEEN_CONTROL_PORT_READ_TRIES = 5; // try every 5 secs.

TorProcess::TorProcess(TorProcessClient *client)
    : m_client(client),  mState(TorProcess::NotStarted), mControlPort(0), mLastTryReadControlPort(0)
{
    mControlPortReadNbTries=0;

}

TorProcess::~TorProcess()
{
    if (state() > NotStarted)
        stop();
}

std::string TorProcess::executable() const
{
    return mExecutable;
}

void TorProcess::setExecutable(const std::string &path)
{
    mExecutable = path;
}

std::string TorProcess::dataDir() const
{
    return mDataDir;
}

void TorProcess::setDataDir(const std::string &path)
{
    mDataDir = path;
}

std::string TorProcess::defaultTorrc() const
{
    return mDefaultTorrc;
}

void TorProcess::setDefaultTorrc(const std::string &path)
{
    mDefaultTorrc = path;
}

std::list<std::string> TorProcess::extraSettings() const
{
    return mExtraSettings;
}

void TorProcess::setExtraSettings(const std::list<std::string> &settings)
{
    mExtraSettings = settings;
}

TorProcess::State TorProcess::state() const
{
    return mState;
}

std::string TorProcess::errorMessage() const
{
    return mErrorMessage;
}

// Does a popen, but dup all file descriptors (STDIN STDOUT and STDERR) to the
// FDs supplied by the parent process

int popen3(int fd[3],const char **const cmd,pid_t& pid)
{
    RsErr() << "Launching Tor in background..." ;

    int i, e;
    int p[3][2];
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
    if(pid)
    {
        // parent
        fd[STDIN_FILENO] = p[STDIN_FILENO][1];
        close(p[STDIN_FILENO][0]);
        fd[STDOUT_FILENO] = p[STDOUT_FILENO][0];
        close(p[STDOUT_FILENO][1]);
        fd[STDERR_FILENO] = p[STDERR_FILENO][0];
        close(p[STDERR_FILENO][1]);
        // success
        return 0;
    }
    else
    {
        RsErr() << "Launching sub-process..." ;
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
    }

error:
    e = errno;
    // preserve original error
    RsErr() << "An error occurred while trying to launch tor in background." ;
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

    mErrorMessage.clear();

    if (mExecutable.empty() || mDataDir.empty()) {
        mErrorMessage = "Tor executable and data directory not specified";
        mState = Failed;

        if(m_client) m_client->processStateChanged(mState); // emit stateChanged(d->state);
        if(m_client) m_client->processErrorChanged(mErrorMessage); // emit errorMessageChanged(d->errorMessage);
        return;
    }

    if (!ensureFilesExist()) {
        mState = Failed;
        if(m_client) m_client->processErrorChanged(mErrorMessage);// emit errorMessageChanged(d->errorMessage);
        if(m_client) m_client->processStateChanged(mState);// emit stateChanged(d->state);
        return;
    }

    ByteArray password = controlPassword();
    ByteArray hashedPassword = torControlHashedPassword(password);

    if (password.empty() || hashedPassword.empty()) {
        mErrorMessage = "Random password generation failed";
        mState = Failed;
        if(m_client) m_client->processErrorChanged(mErrorMessage);// emit errorMessageChanged(d->errorMessage);
        if(m_client) m_client->processStateChanged(mState); // emit stateChanged(d->state);
    }
    else
        RsDbg() << "Using ControlPasswd=\"" << password.toString() << "\", hashed version=\"" << hashedPassword.toString() << "\"" ;

    mState = Starting;

    if(m_client) m_client->processStateChanged(mState);// emit stateChanged(d->state);

    if (RsDirUtil::fileExists(controlPortFilePath()))
        RsDirUtil::removeFile(controlPortFilePath());

    mControlPort = 0;
    mControlHost.clear();

    // Launch the process

    std::vector<std::string> args;

    args.push_back(mExecutable);

    if (!mDefaultTorrc.empty())
    {
        args.push_back("--defaults-torrc");
        args.push_back(mDefaultTorrc);
    }

    args.push_back("-f");
    args.push_back(torrcPath());

    args.push_back("DataDirectory") ;
    args.push_back(mDataDir);

    args.push_back("HashedControlPassword") ;
    args.push_back(hashedPassword.toString());

    args.push_back("ControlPort") ;
    args.push_back("auto");

    args.push_back("ControlPortWriteToFile");
    args.push_back(controlPortFilePath());

    args.push_back("__OwningControllerProcess") ;
    args.push_back(RsUtil::NumberToString(getpid()));

    for(auto s:mExtraSettings)
        args.push_back(s);

    const char *arguments[args.size()+1];
    int n=0;

    // We first pushed everything into a vector of strings to save the pointers obtained from string returning methods
    // by the time the process is launched.

    for(uint32_t i=0;i<args.size();++i)
        arguments[n++]= args[i].data();

    arguments[n] = nullptr;

    int fd[3];  // File descriptors array

    if(popen3(fd,arguments,mTorProcessId))
    {
        RsErr() << "Could not start Tor process. errno=" << errno ;
        mState = Failed;
        return;	// stop the control thread
    }

    int flags ;
    unix_fcntl_nonblock(fd[STDOUT_FILENO]);
    unix_fcntl_nonblock(fd[STDERR_FILENO]);

    mStdOutFD = new RsFdBinInterface(fd[STDOUT_FILENO]);
    mStdErrFD = new RsFdBinInterface(fd[STDERR_FILENO]);
}

void TorProcess::tick()
{
    mStdOutFD->tick();
    mStdErrFD->tick();

    unsigned char buff[1024];
    int s;

    if((s=mStdOutFD->readline(buff,1024))) logMessage(std::string((char*)buff,s));
    if((s=mStdErrFD->readline(buff,1024))) logMessage(std::string((char*)buff,s));

    if(!mStdOutFD->isactive() && !mStdErrFD->isactive())
    {
        RsErr() << "Tor process died. Exiting TorControl process." ;
        stop();
        return;
    }
    time_t now = time(nullptr);

    if(mControlPortReadNbTries <= 10 && (mControlPort==0 || mControlHost.empty()) && mLastTryReadControlPort + INTERVAL_BETWEEN_CONTROL_PORT_READ_TRIES < now)
    {
        mLastTryReadControlPort = now;

        if(tryReadControlPort())
        {
            mState = Ready;
            m_client->processStateChanged(mState);// stateChanged(mState);
        }
        else if(mControlPortReadNbTries > 10)
        {
            //errorMessageChanged(errorMessage);
            //stateChanged(state);
            mState = Failed;
            m_client->processStateChanged(mState);// stateChanged(mState);
        }
    }
}

void TorProcess::stop()
{
    if (state() < Starting)
        return;

    while(mState == Starting)
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

    kill(mTorProcessId,SIGTERM);

    RsInfo() << "Tor process has been normally terminated. Exiting.";

    mState = NotStarted;

    if(m_client) m_client->processStateChanged(mState);// emit stateChanged(d->state);
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
    if (mControlPassword.empty())
        mControlPassword = RsRandom::printable(16);

    return mControlPassword;
}

std::string TorProcess::controlHost()
{
    return mControlHost;
}

unsigned short TorProcess::controlPort()
{
    return mControlPort;
}

bool TorProcess::ensureFilesExist()
{
    if(!RsDirUtil::checkCreateDirectory(mDataDir))
    {
        mErrorMessage = "Cannot create Tor data directory: " + mDataDir;
        return false;
    }

    if (!RsDirUtil::fileExists(torrcPath()))
    {
        FILE *f = RsDirUtil::rs_fopen(torrcPath().c_str(),"w");

        if(!f)
        {
            mErrorMessage = "Cannot create Tor configuration file: " + torrcPath();
            return false;
        }
        else
            fclose(f);
    }

    return true;
}

std::string TorProcess::torrcPath() const
{
    //return QDir::toNativeSeparators(dataDir) + QDir::separator() + QStringLiteral("torrc");
    return RsDirUtil::makePath(mDataDir,"torrc");
}

std::string TorProcess::controlPortFilePath() const
{
    //return QDir::toNativeSeparators(dataDir) + QDir::separator() + QStringLiteral("control-port");
    return RsDirUtil::makePath(mDataDir,"control-port");
}

bool TorProcess::tryReadControlPort()
{
    FILE *file = RsDirUtil::rs_fopen(controlPortFilePath().c_str(),"r");
    std::cerr << "Trying to read control port" << std::endl;

    if(file)
    {
        char *line = nullptr;
        size_t tmp_buffsize = 0;

        size_t size = getline(&line,&tmp_buffsize,file);
        ByteArray data = ByteArray((unsigned char*)line,size).trimmed();
        free(line);

        int p;
        if (data.startsWith("PORT=") && (p = data.lastIndexOf(':')) > 0) {
            mControlHost = data.mid(5, p - 5).toString();
            mControlPort = data.mid(p+1).toInt();

            if (!mControlHost.empty() && mControlPort > 0)
            {
                std::cerr << "Got control host/port = " << mControlHost << ":" << mControlPort << std::endl;
                return true;
            }
        }
    }
    return false;
}
#ifdef TO_REMOVE
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


#endif
