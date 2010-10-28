/*
 * libretroshare/src/tests/services distrib_services.cc
 *
 * RetroShare Service Testing
 *
 * Copyright 2010 by Chris Evi-Parker, Robert Fernie
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 *
 * Please report all bugs and problems to "retroshare@lunamutt.com".
 *
 */

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <list>

#include <openssl/bio.h>
#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>


/**************** PQI_USE_XPGP ******************/
#if defined(PQI_USE_XPGP)
	#include <openssl/xPGP.h>
#else /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/

#endif /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/

#include <openssl/ssl.h>

#include <openssl/err.h>
#include <openssl/rand.h>


#include "distrib_services.h"


RsForum_Test::RsForum_Test() :
TEST_CREATE_FORUMS("Test Create Forums")
{

	bio_err=BIO_new_fp(stderr,BIO_NOCLOSE);

	/* enable memory leak checking unless explicitly disabled */
	CRYPTO_malloc_debug_init();
	CRYPTO_set_mem_debug_options(V_CRYPTO_MDEBUG_ALL);
	CRYPTO_mem_ctrl(CRYPTO_MEM_CHECK_ON);

	SSL_library_init();
	SSL_load_error_strings();

	forum = NULL;


}

RsForum_Test::~RsForum_Test()
{
	/* cleanup */
	CRYPTO_cleanup_all_ex_data();
	ERR_free_strings();
	ERR_remove_state(0);
	EVP_cleanup();
	//CRYPTO_mem_leaks(bio_err);
	if (bio_err != NULL) BIO_free(bio_err);


}

void RsForum_Test::result(std::map<std::string, bool>& results)
{

	return;
}




bool RsForum_Test::setTestCases(std::list<std::string>& testCases)
{
	return false;
}

bool RsForum_Test::runCases(std::list<std::string>& cases)
{
	return false;
}

void RsForum_Test::loadDummyData()
{
	return;

}

bool RsForum_Test::testCreateForums()
{

	std::string fId1 = forum->createForum(L"Forum 1", L"first forum", RS_DISTRIB_PUBLIC);

	forum->tick(); /* expect group publish */
#ifdef WIN32
		Sleep(1000);
#else
		sleep(1);
#endif
	forum->tick();
#ifdef WIN32
		Sleep(1000);
#else
		sleep(1);
#endif

	std::string fId2 = forum->createForum(L"Forum 2", L"next first forum", RS_DISTRIB_PRIVATE);

	forum->tick(); /* expect group publish */
#ifdef WIN32
		Sleep(1000);
#else
		sleep(1);
#endif
	forum->tick();
#ifdef WIN32
		Sleep(1000);
#else
		sleep(1);
#endif

	std::string mId1 = forum->createForumMsg(fId2, "", L"Forum 2 Msg 1", L"first forum msg", true);

	forum->tick(); /* expect msg publish */
#ifdef WIN32
		Sleep(1000);
#else
		sleep(1);
#endif
	forum->tick();
#ifdef WIN32
		Sleep(1000);
#else
		sleep(1);
#endif

	std::string mId2 = forum->createForumMsg(fId2, "", L"Forum 2 Msg 2", L"second forum msg", false);

	forum->tick(); /* expect msg publish */
#ifdef WIN32
		Sleep(1000);
#else
		sleep(1);
#endif
	forum->tick();
#ifdef WIN32
		Sleep(1000);
#else
		sleep(1);
#endif
		return 1 ;

}
