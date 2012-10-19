/* This is a sample implementation of a libssh based SSH server */
/*
Copyright 2003-2009 Aris Adamantiadis

This file is part of the SSH Library

You are free to copy this file, modify it in any way, consider it being public
domain. This does not apply to the rest of the library though, but it is
allowed to cut-and-paste working code from this file to any license of
program.
The goal is to show the API in action. It's not a reference on how terminal
clients must be made or how a client should react.
*/

/*****
 * Heavily Modified by Robert Fernie 2012... for retroshare project!
 *
 */


#ifndef RS_SSHD_INTERFACE_H
#define RS_SSHD_INTERFACE_H

#include <libssh/libssh.h>
#include <libssh/server.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// From inside libretroshare.a
#include "util/rsthreads.h"

#include <string>
#include <map>

#include "rpcsystem.h"

#ifndef KEYS_FOLDER
#ifdef _WIN32
#define KEYS_FOLDER
#else
#define KEYS_FOLDER "/etc/ssh/"
#endif
#endif



/******
 *
 * Minimal Options to start with
 *
 */



//#define ALLOW_CLEARPWDS		1

class RsSshd;
extern RsSshd *rsSshd;


  	// TODO: NB: THIS FN DOES NOT USE A "SLOW" HASH FUNCTION.
        // THE FIRST HALF OF THE HASH STRING IS THE SALT 
int CheckPasswordHash(std::string pwdHashRadix64, std::string password);
int GeneratePasswordHash(std::string saltBin, std::string password, std::string &pwdHashRadix64);
int GenerateSalt(std::string &saltBin);

class RsSshd: public RsThread, public RpcComms
{
public:

// NB: This must be called EARLY before all the threads are launched.
static  RsSshd *InitRsSshd(const std::string &portstr, const std::string &rsakeyfile);


	// Interface.
int 	setRpcSystem(RpcSystem *s);
int 	adduserpwdhash(std::string username, std::string hash);

	// RsThreads Interface.
	virtual void run(); /* called once the thread is started */

	// RsComms Interface.
        virtual int isOkay();
        virtual int error(uint32_t chan_id, std::string msg);

        virtual int active_channels(std::list<uint32_t> &chan_ids);
        virtual int recv_ready(uint32_t chan_id);

        virtual int recv(uint32_t chan_id, uint8_t *buffer, int bytes);
        virtual int recv(uint32_t chan_id, std::string &buffer, int bytes);
        virtual int recv_blocking(uint32_t chan_id, uint8_t *buffer, int bytes);
        virtual int recv_blocking(uint32_t chan_id, std::string &buffer, int bytes);

        virtual int send(uint32_t chan_id, uint8_t *buffer, int bytes);
        virtual int send(uint32_t chan_id, const std::string &buffer);

	virtual int setSleepPeriods(float busy, float idle);

private:
	RsSshd(std::string portStr); /* private constructor => so can only create with */

int 	init(const std::string &pathrsakey);

	// High level operations.
int 	listenConnect();
int 	setupSession();
int 	interactive();

	// Lower Level Operations.
int 	authUser();
int	setupChannel();
int	setupShell();
int	doEcho();

	// Terminal Handling!
//int 	doTermServer();
int 	doRpcSystem();

int 	cleanupSession();
int 	cleanupAll();

	/* Password Checking */
int 	auth_password(const char *name, const char *pwd);
int 	auth_password_hashed(const char *name, const char *pwd);
#ifdef ALLOW_CLEARPWDS
int 	auth_password_basic(char *name, char *pwd);
#endif // ALLOW_CLEARPWDS

	// DATA.

	RsMutex mSshMtx;

       	uint32_t mBusyUSleep;
        uint32_t mIdleUSleep;
	
	uint32_t mState;
	uint32_t mBindState;

	std::string mPortStr;
	ssh_session mSession;
	ssh_bind mBind;
	ssh_channel mChannel;

	RpcSystem *mRpcSystem;

#ifdef ALLOW_CLEARPWDS
	std::map<std::string, std::string> mPasswords;
#endif // ALLOW_CLEARPWDS
	std::map<std::string, std::string> mPwdHashs;

};


#endif

