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



#define ALLOW_CLEARPWDS		1

class RsSshd;
extern RsSshd *rsSshd;

class RsSshd: public RsThread
{
public:

  	// TODO: NB: THIS FN DOES NOT USE A "SLOW" HASH FUNCTION.
        // THE FIRST HALF OF THE HASH STRING IS THE SALT 
int adduserpwdhash(std::string username, std::string hash);
#ifdef ALLOW_CLEARPWDS
int adduser(std::string username, std::string password);
#endif // ALLOW_CLEARPWDS



virtual void run(); /* overloaded from RsThread => called once the thread is started */

// NB: This must be called EARLY before all the threads are launched.
static  RsSshd *InitRsSshd(uint16_t port, std::string rsakeyfile);

private:
	RsSshd(uint16_t port); /* private constructor => so can only create with */

int 	init(std::string pathrsakey);

	// High level operations.
int 	listenConnect();
int 	setupSession();
int 	interactive();

	// Lower Level Operations.
int 	authUser();
int	setupChannel();
int	setupShell();
int	doEcho();

int 	cleanupSession();
int 	cleanupAll();

	/* Password Checking */
int 	auth_password(char *name, char *pwd);
int 	auth_password_hashed(char *name, char *pwd);
#ifdef ALLOW_CLEARPWDS
int 	auth_password_basic(char *name, char *pwd);
#endif // ALLOW_CLEARPWDS

	// DATA.

	RsMutex mSshMtx;
	
	uint32_t mState;
	uint32_t mBindState;

	uint16_t mPort;
	ssh_session mSession;
	ssh_bind mBind;
	ssh_channel mChannel;

#ifdef ALLOW_CLEARPWDS
	std::map<std::string, std::string> mPasswords;
#endif // ALLOW_CLEARPWDS
	std::map<std::string, std::string> mPwdHashs;

};


#endif

