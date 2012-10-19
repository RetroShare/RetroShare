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


#include <libssh/callbacks.h>

#include "ssh/rssshd.h"

#include <iostream>

#define RSSSHD_STATE_NULL	0
#define RSSSHD_STATE_INIT_OK	1
#define RSSSHD_STATE_CONNECTED  2
#define RSSSHD_STATE_ERROR	3

RsSshd *rsSshd = NULL; // External Reference Variable.

// NB: This must be called EARLY before all the threads are launched.
RsSshd *RsSshd::InitRsSshd(const std::string &portStr, const std::string &rsakeyfile)
{
#if LIBSSH_VERSION_INT >= SSH_VERSION_INT(0,5,0)
	ssh_threads_set_callbacks(ssh_threads_get_pthread());
#endif
	ssh_init();

	rsSshd = new RsSshd(portStr);
	if (rsSshd->init(rsakeyfile))
	{
		return rsSshd;
	}

	rsSshd = NULL;
	return rsSshd;
}


RsSshd::RsSshd(std::string portStr)
:mSshMtx("sshMtx"), mPortStr(portStr), mChannel(0)
{

	mState = RSSSHD_STATE_NULL;
    	mBindState = 0;
        mRpcSystem = NULL;

	mSession = NULL;

	setSleepPeriods(0.01, 0.1);
	return;
}



int RsSshd::init(const std::string &pathrsakey)
{

    mBind=ssh_bind_new();

    //ssh_bind_options_set(mBind, SSH_BIND_OPTIONS_DSAKEY, KEYS_FOLDER "ssh_host_dsa_key");
    //ssh_bind_options_set(mBind, SSH_BIND_OPTIONS_RSAKEY, KEYS_FOLDER "ssh_host_rsa_key");

    //ssh_bind_options_set(mBind, SSH_BIND_OPTIONS_BINDPORT_STR, arg);
    ssh_bind_options_set(mBind, SSH_BIND_OPTIONS_BINDPORT_STR, mPortStr.c_str());
    //ssh_bind_options_set(mBind, SSH_BIND_OPTIONS_DSAKEY, arg);
    //ssh_bind_options_set(mBind, SSH_BIND_OPTIONS_HOSTKEY, arg);
    ssh_bind_options_set(mBind, SSH_BIND_OPTIONS_RSAKEY, pathrsakey.c_str());
    //ssh_bind_options_set(mBind, SSH_BIND_OPTIONS_LOG_VERBOSITY_STR, "3");

    mState = RSSSHD_STATE_INIT_OK;
    mBindState = 0;

	return 1;
}


void RsSshd::run()
{
	/* main loop */

	/* listen */
	bool sshOk = true;
	while(sshOk)
	{
		std::cerr << "RsSshd::run() starting sshd cycle";
		std::cerr << std::endl;

		if (listenConnect())
		{
			std::cerr << "RsSshd::run() success connect => setup mSession";
			std::cerr << std::endl;
		
			if (setupSession())
			{
				std::cerr << "RsSshd::run() setup mSession => interactive";
				std::cerr << std::endl;
		
    				mState = RSSSHD_STATE_CONNECTED;
				interactive();
			}
			else
			{
				std::cerr << "RsSshd::run() setup mSession failed";
				std::cerr << std::endl;
			}
		}	
		cleanupSession();
#ifndef WINDOWS_SYS
		sleep(5); // have a break;
#else
		Sleep(5000); // have a break;
#endif
	}
}


int RsSshd::listenConnect()
{
	std::cerr << "RsSshd::listenConnect()";
	std::cerr << std::endl;

	if (!mBindState)
	{
		if(ssh_bind_listen(mBind)<0)
		{
			printf("Error listening to socket: %s\n",ssh_get_error(mBind));
			return 0;
		}
    		mBindState = 1;
	}
	else
	{
		std::cerr << "RsSshd::listenConnect() Already Bound...";
		std::cerr << std::endl;
	}

    	mSession=ssh_new();
	int r=ssh_bind_accept(mBind,mSession);
	if(r==SSH_ERROR)
	{
		printf("error accepting a connection : %s\n",ssh_get_error(mBind));
		return 0;
	}

#if LIBSSH_VERSION_INT >= SSH_VERSION_INT(0,5,0)
	if (ssh_handle_key_exchange(mSession)) 
#else
	if (ssh_accept(mSession)) 
#endif
	{
		printf("ssh_handle_key_exchange: %s\n", ssh_get_error(mSession));
		return 0;
	}
	return 1;
}


int RsSshd::setupSession()
{
	std::cerr << "RsSshd::listenConnect()";
	std::cerr << std::endl;

	if (authUser())
	{
		std::cerr << "RsSshd::listenConnect() authUser SUCCESS";
		std::cerr << std::endl;

		if (setupChannel())
		{
			std::cerr << "RsSshd::listenConnect() setupChannel SUCCESS";
			std::cerr << std::endl;

			if (setupShell())
			{
				std::cerr << "RsSshd::listenConnect() setupShell SUCCESS";
				std::cerr << std::endl;

				return 1;
			}
		}
	}
	std::cerr << "RsSshd::listenConnect() Failed";
	std::cerr << std::endl;

	return 0;
}


int RsSshd::interactive()
{
	std::cerr << "RsSshd::interactive()";
	std::cerr << std::endl;

	doRpcSystem();
	//doEcho();
	return 1;
}



int RsSshd::authUser()
{
	std::cerr << "RsSshd::authUser()";
	std::cerr << std::endl;


    ssh_message message;
    int auth = 0;

    do {
        message=ssh_message_get(mSession);
        if(!message)
            break;
        switch(ssh_message_type(message)){
            case SSH_REQUEST_AUTH:
                switch(ssh_message_subtype(message)){
                    case SSH_AUTH_METHOD_PASSWORD:
                        printf("User %s wants to auth with pass %s\n",
                               ssh_message_auth_user(message),
                               ssh_message_auth_password(message));
                        if(auth_password(ssh_message_auth_user(message),
                           ssh_message_auth_password(message))){
                               auth=1;
                               ssh_message_auth_reply_success(message,0);
                               break;
                           }
                        // not authenticated, send default message
                    case SSH_AUTH_METHOD_NONE:
                    default:
                        ssh_message_auth_set_methods(message,SSH_AUTH_METHOD_PASSWORD);
                        ssh_message_reply_default(message);
                        break;
                }
                break;
            default:
                ssh_message_reply_default(message);
        }
        ssh_message_free(message);
    } while (!auth);

    if(!auth){
        printf("auth error: %s\n",ssh_get_error(mSession));
        ssh_disconnect(mSession);
        return 0;
    }

	std::cerr << "RsSshd::authuser() Success";
	std::cerr << std::endl;

    return 1;
}


int RsSshd::setupChannel()
{
	std::cerr << "RsSshd::setupChannel()";
	std::cerr << std::endl;

    ssh_message message;


    do {
        message=ssh_message_get(mSession);
        if(message){
            switch(ssh_message_type(message)){
                case SSH_REQUEST_CHANNEL_OPEN:
                    if(ssh_message_subtype(message)==SSH_CHANNEL_SESSION){
                        mChannel=ssh_message_channel_request_open_reply_accept(message);
                        break;
                    }
                default:
                ssh_message_reply_default(message);
            }
            ssh_message_free(message);
        }
    } while(message && !mChannel);
    if(!mChannel){
        printf("error : %s\n",ssh_get_error(mSession));
        ssh_finalize();
        return 0;
    }

    return 1;
}


int RsSshd::setupShell()
{
	std::cerr << "RsSshd::setupShell()";
	std::cerr << std::endl;

    int shell = 0;
    ssh_message message;

    do {
        message=ssh_message_get(mSession);
        if(message && ssh_message_type(message)==SSH_REQUEST_CHANNEL &&
           ssh_message_subtype(message)==SSH_CHANNEL_REQUEST_SHELL){
                shell = 1;
                ssh_message_channel_request_reply_success(message);
                break;
           }
        if(!shell){
            ssh_message_reply_default(message);
        }
        ssh_message_free(message);
    } while (message && !shell);

    if(!shell){
        printf("error : %s\n",ssh_get_error(mSession));
        return 0;
    }

    return 1;
}

// CLEANUP
int RsSshd::cleanupSession()
{
	std::cerr << "RsSshd::cleanupSession()";
	std::cerr << std::endl;

    ssh_disconnect(mSession);
    ssh_free(mSession);
    return 1;
}


int RsSshd::cleanupAll()
{
	std::cerr << "RsSshd::cleanupAll()";
	std::cerr << std::endl;

    cleanupSession();
    if (mBindState)
    {
    	ssh_bind_free(mBind);
    	mBindState = 0;
    }
    ssh_finalize();
    return 1;
}



// Various Operating Modes.
int RsSshd::doEcho()
{
	std::cerr << "RsSshd::doEcho()";
	std::cerr << std::endl;
    int i = 0;
    char buf[2048];

    do{
#if LIBSSH_VERSION_INT >= SSH_VERSION_INT(0,5,0)
        i=ssh_channel_read(mChannel,buf, 2048, 0);
#else
        i=channel_read(mChannel,buf, 2048, 0);
#endif
        if(i>0) {
#if LIBSSH_VERSION_INT >= SSH_VERSION_INT(0,5,0)
            ssh_channel_write(mChannel, buf, i);
            ssh_channel_write(mChannel, buf, i);
#else
            channel_write(mChannel, buf, i);
            channel_write(mChannel, buf, i);
#endif
            if (write(1,buf,i) < 0) {
                printf("error writing to buffer\n");
                return 0;
            }
        }
    } while (i>0);

	std::cerr << "RsSshd::doEcho() Finished";
	std::cerr << std::endl;

	return 1;
}


int RsSshd::setRpcSystem(RpcSystem *s)
{
	mRpcSystem = s;
	return 1;
}


#if 0

int RsSshd::doTermServer()
{
	std::cerr << "RsSshd::doTermServer()";
	std::cerr << std::endl;

	if (!mTermServer)
	{
		std::cerr << "RsSshd::doTermServer() ERROR Not Set";
		std::cerr << std::endl;
		return 0;
	}

	mTermServer->reset(); // clear everything for new user.

	bool okay = true;
	while(okay)
	{
		char buf;
#if LIBSSH_VERSION_INT >= SSH_VERSION_INT(0,5,0)
		int size = ssh_channel_read_nonblocking(mChannel, &buf, 1, 0);
#else
		int size = channel_read_nonblocking(mChannel, &buf, 1, 0);
#endif
		bool haveInput = (size > 0);
		std::string output;

		int rt = mTermServer->tick(haveInput, buf, output);
	
		if (output.size() > 0)
		{
#if LIBSSH_VERSION_INT >= SSH_VERSION_INT(0,5,0)
            		ssh_channel_write(mChannel, output.c_str(), output.size());
#else
            		channel_write(mChannel, output.c_str(), output.size());
#endif
		}

		if (!haveInput)
		{
			/* have a little sleep */
			sleep(1); //0000000); // 1/10th sec.
			//usleep(10000000); // 1/10th sec.
		}
		else
		{
			usleep(10000); // 1/100th sec.
		}

		if (rt < 0)
		{
			okay = false; // exit.
		}	
	}

	std::cerr << "RsSshd::doTermServer() Finished";
	std::cerr << std::endl;

	return 1;
}

#endif


int RsSshd::doRpcSystem()
{
	std::cerr << "RsSshd::doRpcSystem()";
	std::cerr << std::endl;

	if (!mRpcSystem)
	{
		std::cerr << "RsSshd::doRpcSystem() ERROR Not Set";
		std::cerr << std::endl;
		return 0;
	}

	uint32_t dummy_chan_id = 1;
	mRpcSystem->reset(dummy_chan_id); // clear everything for new user.

	bool okay = true;
	while(okay)
	{
		int rt = mRpcSystem->tick();
		if (rt)
		{
			// Working - so small sleep,
			usleep(mBusyUSleep); 
		}
		else
		{
			// No work cycle, longer break.
			usleep(mIdleUSleep); 
		}

		if (rt < 0)
		{
			okay = false; // exit.
		}	

		if (!isOkay())
		{
			okay = false;
		}
	}

	mRpcSystem->reset(dummy_chan_id); // cleanup old channel items.

	std::cerr << "RsSshd::doRpcSystem() Finished";
	std::cerr << std::endl;

	return 1;
}

// RpcComms Interface....
int RsSshd::isOkay()
{
    	return (mState == RSSSHD_STATE_CONNECTED);
}

        std::list<uint32_t>::iterator it;
int RsSshd::active_channels(std::list<uint32_t> &chan_ids)
{
	if (isOkay())
	{
		chan_ids.push_back(1); // dummy for now.
	}

	return 1;
}

int RsSshd::error(uint32_t chan_id, std::string msg)
{
	std::cerr << "RsSshd::error(" << msg << ")";
	std::cerr << std::endl;

    	mState = RSSSHD_STATE_ERROR;
	return 1;	
}


int RsSshd::recv_ready(uint32_t chan_id)
{
	int bytes = ssh_channel_poll(mChannel, 0);
	return bytes;		
}


int RsSshd::recv(uint32_t chan_id, uint8_t *buffer, int bytes)
{
#if LIBSSH_VERSION_INT >= SSH_VERSION_INT(0,5,0)
	int size = ssh_channel_read_nonblocking(mChannel, buffer, bytes, 0);
#else
	int size = channel_read_nonblocking(mChannel, buffer, bytes, 0);
#endif
	return size;
}


int RsSshd::recv(uint32_t chan_id, std::string &buffer, int bytes)
{
	char input[bytes];
#if LIBSSH_VERSION_INT >= SSH_VERSION_INT(0,5,0)
	int size = ssh_channel_read_nonblocking(mChannel, input, bytes, 0);
#else
	int size = channel_read_nonblocking(mChannel, input, bytes, 0);
#endif
	for(int i = 0; i < size; i++)
	{
		buffer += input[i];
	}
	return size;
}


int RsSshd::recv_blocking(uint32_t chan_id, uint8_t *buffer, int bytes)
{
#if LIBSSH_VERSION_INT >= SSH_VERSION_INT(0,5,0)
	int size = ssh_channel_read(mChannel, buffer, bytes, 0);
#else
	int size = channel_read(mChannel, buffer, bytes, 0);
#endif
	return size;
}


int RsSshd::recv_blocking(uint32_t chan_id, std::string &buffer, int bytes)
{
	char input[bytes];
#if LIBSSH_VERSION_INT >= SSH_VERSION_INT(0,5,0)
	int size = ssh_channel_read(mChannel, input, bytes, 0);
#else
	int size = channel_read(mChannel, input, bytes, 0);
#endif
	for(int i = 0; i < size; i++)
	{
		buffer += input[i];
	}
	return size;
}

int RsSshd::send(uint32_t chan_id, uint8_t *buffer, int bytes)
{
#if LIBSSH_VERSION_INT >= SSH_VERSION_INT(0,5,0)
	ssh_channel_write(mChannel, buffer, bytes);
#else
	channel_write(mChannel, buffer, bytes);
#endif
	return 1;
}

int RsSshd::send(uint32_t chan_id, const std::string &buffer)
{
#if LIBSSH_VERSION_INT >= SSH_VERSION_INT(0,5,0)
	ssh_channel_write(mChannel, buffer.c_str(), buffer.size());
#else
	channel_write(mChannel, buffer.c_str(), buffer.size());
#endif
	return 1;
}

int RsSshd::setSleepPeriods(float busy, float idle)
{
	mBusyUSleep = busy * 1000000;
	mIdleUSleep = idle * 1000000;
	return 1;
}



/***********************************************************************************/
    /* PASSWORDS */
/***********************************************************************************/

int RsSshd::auth_password(const char *name, const char *pwd)
{
#ifdef ALLOW_CLEARPWDS
	if (auth_password_basic(name, pwd))
		return 1;
#endif // ALLOW_CLEARPWDS

	if (auth_password_hashed(name, pwd))
		return 1;
	return 0;
}

#define RSSSHD_MIN_PASSWORD	4
#define RSSSHD_MIN_USERNAME	3

#ifdef ALLOW_CLEARPWDS
int RsSshd::adduser(std::string username, std::string password)
{
	if (password.length() < RSSSHD_MIN_PASSWORD)
	{
		std::cerr << "RsSshd::adduser() Password Too Short";
		return 0;
	}

	if (username.length() < RSSSHD_MIN_USERNAME)
	{
		std::cerr << "RsSshd::adduser() Password Too Short";
		return 0;
	}

	std::map<std::string, std::string>::iterator it;
	it = mPasswords.find(username);
	if (it != mPasswords.end())
	{
		std::cerr << "RsSshd::adduser() Warning username already exists";
	}

	mPasswords[username] = password;

	return 1;
}


int RsSshd::auth_password_basic(char *name, char *pwd)
{
	std::string username(name);
	std::string password(pwd);
	
	std::map<std::string, std::string>::iterator it;
	it = mPasswords.find(username);
	if (it == mPasswords.end())
	{
		std::cerr << "RsSshd::auth_password() Unknown username";
		return 0;
	}

	if (it->second == password)
	{
		std::cerr << "RsSshd::auth_password() logged in " << username;
		return 1;
	}

	std::cerr << "RsSshd::auth_password() Invalid pwd for " << username;
	return 0;
}
#endif // ALLOW_CLEARPWDS

//#define RSSSHD_HASH_PWD_LENGTH		40

int RsSshd::adduserpwdhash(std::string username, std::string hash)
{
#if 0
	if (hash.length() != RSSSHD_HASH_PWD_LENGTH)
	{
		std::cerr << "RsSshd::adduserpwdhash() Hash Wrong Length";
		return 0;
	}
#endif

	if (username.length() < RSSSHD_MIN_USERNAME)
	{
		std::cerr << "RsSshd::adduserpwdhash() Username Too Short";
		return 0;
	}

	std::map<std::string, std::string>::iterator it;
	it = mPwdHashs.find(username);
	if (it != mPwdHashs.end())
	{
		std::cerr << "RsSshd::adduser() Warning username already exists";
	}

	mPwdHashs[username] = hash;

	return 1;
}


int RsSshd::auth_password_hashed(const char *name, const char *pwd)
{
	std::string username(name);
	std::string password(pwd);
	
	std::map<std::string, std::string>::iterator it;
	it = mPwdHashs.find(username);
	if (it == mPwdHashs.end())
	{
		std::cerr << "RsSshd::auth_password_hashed() Unknown username";
		return 0;
	}

	if (CheckPasswordHash(it->second, password))
	{
		std::cerr << "RsSshd::auth_password_hashed() logged in " << username;
		return 1;
	}

	std::cerr << "RsSshd::auth_password_hashed() Invalid pwd for " << username;
	return 0;
}

#include "util/radix64.h"
#include "util/rsrandom.h"
#include <openssl/evp.h>

#define RSSSHD_PWD_SALT_LEN 16
#define RSSSHD_PWD_MIN_LEN 8


#if 0
int printHex(const char *data, int len)
{
	for(int i = 0; i < len; i++)
	{
		fprintf(stderr, "%02x", (uint8_t) data[i]);
	}
	return 1;
}
#endif



int GenerateSalt(std::string &saltBin)
{
	/* get from random */
	for(int i = 0; i < RSSSHD_PWD_SALT_LEN / 4; i++)
	{
		uint32_t rnd = RSRandom::random_u32();
		saltBin += ((char *) &rnd)[0];
		saltBin += ((char *) &rnd)[1];
		saltBin += ((char *) &rnd)[2];
		saltBin += ((char *) &rnd)[3];
	}

#if 0
	std::cerr << "HexSalt: ";
	printHex(saltBin.c_str(), saltBin.size());
	std::cerr << std::endl;
#endif

	return 1;
}

int GeneratePasswordHash(std::string saltBin, std::string password, std::string &pwdHashRadix64)
{
#if 0
	std::cerr << "GeneratePasswordHash()";
	std::cerr << std::endl;

	std::cerr << "HexSalt: ";
	printHex(saltBin.c_str(), saltBin.size());
	std::cerr << std::endl;
#endif

	if (saltBin.size() != RSSSHD_PWD_SALT_LEN)
	{
		return 0;
	}

	if (password.size() < RSSSHD_PWD_MIN_LEN)
	{
		return 0;
	}

	EVP_MD_CTX *ctx = EVP_MD_CTX_create();
	EVP_DigestInit(ctx, EVP_sha256());

	EVP_DigestUpdate(ctx, saltBin.c_str(), saltBin.size());
	EVP_DigestUpdate(ctx, password.c_str(), password.size());


	unsigned char hash[1024];
	unsigned int  s = 1024 - RSSSHD_PWD_SALT_LEN;

	for(int i = 0; i < RSSSHD_PWD_SALT_LEN; i++)
	{
		hash[i] = saltBin[i];
	}

	EVP_DigestFinal(ctx, &(hash[RSSSHD_PWD_SALT_LEN]), &s);

	Radix64::encode((char *)hash, s + RSSSHD_PWD_SALT_LEN, pwdHashRadix64);

#if 0
	std::cerr << "Salt Length: " << RSSSHD_PWD_SALT_LEN;
	std::cerr << std::endl;
	std::cerr << "Hash Length: " << s;
	std::cerr << std::endl;
	std::cerr << "Total Length: " << s + RSSSHD_PWD_SALT_LEN;
	std::cerr << std::endl;


	std::cerr << "Encoded Length: " << pwdHashRadix64.size();
	std::cerr << std::endl;

	std::cerr << "GeneratePasswordHash() Output: " << pwdHashRadix64;
	std::cerr << std::endl;
#endif

	return 1;
}

	
int CheckPasswordHash(std::string pwdHashRadix64, std::string password)
{
	char output[1024];
	char *buf = NULL;
	size_t len = 1024;
	Radix64::decode(pwdHashRadix64, buf, len);
	for(unsigned int i = 0; (i < len) && (i < 1024); i++)
	{
		output[i] = buf[i];
	}
	delete []buf;

#if 0
	std::cerr << "CheckPasswordHash() Input: " << pwdHashRadix64;
	std::cerr << std::endl;
	std::cerr << "Decode Length: " << len;
	std::cerr << std::endl;
	std::cerr << "HexDecoded: ";
	printHex(output, len);
	std::cerr << std::endl;
#endif

	/* first N bytes are SALT */
	EVP_MD_CTX *ctx = EVP_MD_CTX_create();
	EVP_DigestInit(ctx, EVP_sha256());

	EVP_DigestUpdate(ctx, output, RSSSHD_PWD_SALT_LEN);
	EVP_DigestUpdate(ctx, password.c_str(), password.size());

#if 0
	std::cerr << "HexSalt: ";
	printHex(output, RSSSHD_PWD_SALT_LEN);
	std::cerr << std::endl;
#endif

	unsigned char hash[128];
	unsigned int  s = 128;
	EVP_DigestFinal(ctx, hash, &s);

	/* Final Comparison */
	if (s != len - RSSSHD_PWD_SALT_LEN)
	{
		std::cerr << "Length Mismatch";
		return 0;
	}

	if (0 == strncmp(&(output[RSSSHD_PWD_SALT_LEN]), (char *) hash, s))
	{
		return 1;
	}

	return 0;
}



