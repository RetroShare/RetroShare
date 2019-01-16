/*
 * bss_tou.c Based on bss_*.c from OpenSSL Library....
 * Therefore - released under their licence.
 * Copyright 2004-2006 by Robert Fernie. (retroshare@lunamutt.com)
 */

/* Copyright (C) 1995-1998 Eric Young (eay@cryptsoft.com)
 * All rights reserved.
 *
 * This package is an SSL implementation written
 * by Eric Young (eay@cryptsoft.com).
 * The implementation was written so as to conform with Netscapes SSL.
 * 
 * This library is free for commercial and non-commercial use as long as
 * the following conditions are aheared to.  The following conditions
 * apply to all code found in this distribution, be it the RC4, RSA,
 * lhash, DES, etc., code; not just the SSL code.  The SSL documentation
 * included with this distribution is covered by the same copyright terms
 * except that the holder is Tim Hudson (tjh@cryptsoft.com).
 * 
 * Copyright remains Eric Young's, and as such any Copyright notices in
 * the code are not to be removed.
 * If this package is used in a product, Eric Young should be given attribution
 * as the author of the parts of the library used.
 * This can be in the form of a textual message at program startup or
 * in documentation (online or textual) provided with the package.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *    "This product includes cryptographic software written by
 *     Eric Young (eay@cryptsoft.com)"
 *    The word 'cryptographic' can be left out if the rouines from the library
 *    being used are not cryptographic related :-).
 * 4. If you include any Windows specific code (or a derivative thereof) from 
 *    the apps directory (application code) you must include an acknowledgement:
 *    "This product includes software written by Tim Hudson (tjh@cryptsoft.com)"
 * 
 * THIS SOFTWARE IS PROVIDED BY ERIC YOUNG ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * 
 * The licence and distribution terms for any publically available version or
 * derivative of this code cannot be changed.  i.e. this code cannot simply be
 * copied and put under another distribution licence
 * [including the GNU Public Licence.]
 */

#ifndef OPENSSL_NO_SOCK

#include "bio_tou.h"

// STUFF defined in the header.......
//int BIO_tou_socket_should_retry(int s, int e);
//int BIO_tou_socket_non_fatal_error(int error);
//#define BIO_TYPE_TOU_SOCKET     (30|0x0400|0x0100)      /* NEW rmfern type */
//BIO_METHOD *BIO_s_tou_socket(void);

#include <stdio.h>
#include <errno.h>
#define USE_SOCKETS
//#include "cryptlib.h"
#include <openssl/bio.h>
#include <string.h> /* for strlen() */

#define DEBUG_TOU_BIO 1

static int tou_socket_write(BIO *h, const char *buf, int num);
static int tou_socket_read(BIO *h, char *buf, int size);
static int tou_socket_puts(BIO *h, const char *str);
static long tou_socket_ctrl(BIO *h, int cmd, long arg1, void *arg2);
static int tou_socket_new(BIO *h);
static int tou_socket_free(BIO *data);
static int get_last_tou_socket_error(int s);
static int clear_tou_socket_error(int s);

#include "tou.h"

#if OPENSSL_VERSION_NUMBER < 0x10100000L || defined(LIBRESSL_VERSION_NUMBER)

static int  BIO_get_init(BIO *a) { return a->init; }

#if (defined(LIBRESSL_VERSION_NUMBER) && LIBRESSL_VERSION_NUMBER < 0x20700000) || (!defined(LIBRESSL_VERSION_NUMBER) && OPENSSL_VERSION_NUMBER < 0x10100000L)

static int  BIO_get_shutdown(BIO *a) { return a->shutdown; }
static void BIO_set_init(BIO *a,int i) { a->init=i; }
static void BIO_set_data(BIO *a,void *p) { a->ptr = p; }
long (*BIO_meth_get_ctrl(const BIO_METHOD* biom)) (BIO*, int, long, void*)
{ return biom->ctrl; }

#endif

#else

typedef struct bio_method_st {
    int type;
    const char *name;
    int (*bwrite) (BIO *, const char *, int);
    int (*bread) (BIO *, char *, int);
    int (*bputs) (BIO *, const char *);
    int (*bgets) (BIO *, char *, int);
    long (*ctrl) (BIO *, int, long, void *);
    int (*create) (BIO *);
    int (*destroy) (BIO *);
    long (*callback_ctrl) (BIO *, int, bio_info_cb *);
} BIO_METHOD;

#endif

static BIO_METHOD methods_tou_sockp =
{
    BIO_TYPE_TOU_SOCKET,
    "tou_socket",
    tou_socket_write,
    tou_socket_read,
    tou_socket_puts,
    NULL, /* tou_gets, */
    tou_socket_ctrl,
    tou_socket_new,
    tou_socket_free,
    NULL,
};

BIO_METHOD* BIO_s_tou_socket(void)
{
#ifdef DEBUG_TOU_BIO
	fprintf(stderr, "BIO_s_tou_socket(void)\n");
#endif
	return(&methods_tou_sockp);
}

#else

BIO_METHOD* BIO_s_tou_socket(void)
{
	static BIO_METHOD* methods_tou_sockp_ptr = NULL;
	if(!methods_tou_sockp_ptr)
	{
		 methods_tou_sockp_ptr = BIO_meth_new(BIO_TYPE_TOU_SOCKET, "tou_socket");

		BIO_meth_set_write(   methods_tou_sockp_ptr, tou_socket_write );
		BIO_meth_set_read(    methods_tou_sockp_ptr, tou_socket_read  );
		BIO_meth_set_puts(    methods_tou_sockp_ptr, tou_socket_puts  );
		BIO_meth_set_ctrl(    methods_tou_sockp_ptr, tou_socket_ctrl  );
		BIO_meth_set_create(  methods_tou_sockp_ptr, tou_socket_new   );
		BIO_meth_set_destroy( methods_tou_sockp_ptr, tou_socket_free  );
	}

	return methods_tou_sockp_ptr;
}

#endif

BIO *BIO_new_tou_socket(int fd, int close_flag)
	{
	BIO *ret;
#ifdef DEBUG_TOU_BIO
	fprintf(stderr, "BIO_new_tou_socket(%d)\n", fd);
#endif

	ret=BIO_new(BIO_s_tou_socket());
	if (ret == NULL) return(NULL);
	BIO_set_fd(ret,fd,close_flag);
	return(ret);
	}

static int tou_socket_new(BIO *bi)
	{
#ifdef DEBUG_TOU_BIO
	fprintf(stderr, "tou_socket_new()\n");
#endif
    BIO_set_init(bi,0) ;
    BIO_set_data(bi,NULL) ;	// sets bi->ptr
    BIO_set_flags(bi,0) ;
    BIO_set_fd(bi,0,0) ;
	return(1);
	}

static int tou_socket_free(BIO *a)
	{
#ifdef DEBUG_TOU_BIO
	fprintf(stderr, "tou_socket_free()\n");
#endif
	if (a == NULL) return(0);

    if(BIO_get_shutdown(a))
		{
		if(BIO_get_init(a))
			{
			tou_close(BIO_get_fd(a,NULL));
			}
        BIO_set_init(a,0) ;
        BIO_set_flags(a,0) ;
		}
	return(1);
	}
	
static int tou_socket_read(BIO *b, char *out, int outl)
	{
	int ret=0;
#ifdef DEBUG_TOU_BIO
	fprintf(stderr, "tou_socket_read(%p,%p,%d)\n",b,out,outl);
#endif

	if (out != NULL)
		{
		clear_tou_socket_error(BIO_get_fd(b,NULL));
		/* call tou library */
		ret=tou_read(BIO_get_fd(b,NULL),out,outl);
		BIO_clear_retry_flags(b);
		if (ret <= 0)
			{
			if (BIO_tou_socket_should_retry(BIO_get_fd(b,NULL), ret))
				BIO_set_retry_read(b);
			}
		}
#ifdef DEBUG_TOU_BIO
	fprintf(stderr, "tou_socket_read() = %d\n", ret);
#endif
	return(ret);
	}

static int tou_socket_write(BIO *b, const char *in, int inl)
	{
	int ret;
#ifdef DEBUG_TOU_BIO
	fprintf(stderr, "tou_socket_write(%p,%p,%d)\n",b,in,inl);
#endif
	
	clear_tou_socket_error(BIO_get_fd(b,NULL));
	/* call tou library */
	ret=tou_write(BIO_get_fd(b,NULL),in,inl);
	BIO_clear_retry_flags(b);
	if (ret <= 0)
		{
		if (BIO_tou_socket_should_retry(BIO_get_fd(b,NULL),ret))
		{
			BIO_set_retry_write(b);
#ifdef DEBUG_TOU_BIO
			fprintf(stderr, "tou_socket_write() setting retry flag\n");
#endif
		}
		}
#ifdef DEBUG_TOU_BIO
	fprintf(stderr, "tou_socket_write() = %d\n", ret);
#endif
	return(ret);
	}

static long tou_socket_ctrl(BIO *b, int cmd, long num, void *ptr)
	{
	long ret=1;
#ifdef DEBUG_TOU_BIO
	fprintf(stderr, "tou_socket_ctrl(%p,%d,%ld)\n", b, cmd, num);
#endif

	// We are not allowed to call BIO_set_fd here, because it will trigger a callback, which re-ends here

	switch (cmd)
		{
	case BIO_CTRL_RESET:
		num=0;
		/* fallthrough */
	case BIO_C_FILE_SEEK:
		ret=0;
		break;
	case BIO_C_FILE_TELL:
	case BIO_CTRL_INFO:
		ret=0;
		break;
	case BIO_C_SET_FD:
		tou_socket_free(b);
		ret = BIO_meth_get_ctrl((BIO_METHOD*)BIO_s_fd())(b,cmd,num,ptr);
		break;
	case BIO_C_GET_FD:
		ret = BIO_meth_get_ctrl((BIO_METHOD*)BIO_s_fd())(b,cmd,num,ptr);
		break;
	case BIO_CTRL_GET_CLOSE:
		ret = BIO_meth_get_ctrl((BIO_METHOD*)BIO_s_fd())(b,cmd,num,ptr);
		break;
	case BIO_CTRL_SET_CLOSE:
		ret = BIO_meth_get_ctrl((BIO_METHOD*)BIO_s_fd())(b,cmd,num,ptr);
		break;
	case BIO_CTRL_PENDING:
		ret = tou_maxread(BIO_get_fd(b,NULL));
#ifdef DEBUG_TOU_BIO
		fprintf(stderr, "tou_pending = %ld\n", ret);
#endif
		break;
	case BIO_CTRL_WPENDING:
		ret = tou_maxwrite(BIO_get_fd(b,NULL));
#ifdef DEBUG_TOU_BIO
		fprintf(stderr, "tou_wpending = %ld\n", ret);
#endif
		break;
	case BIO_CTRL_DUP:
	case BIO_CTRL_FLUSH:
		ret=1;
		break;
	default:
		ret=0;
		break;
		}
	return(ret);
	}

static int tou_socket_puts(BIO *bp, const char *str)
	{
	int n,ret;

#ifdef DEBUG_TOU_BIO
	fprintf(stderr, "tou_socket_puts()\n");
#endif

	n=strlen(str);
	ret=tou_socket_write(bp,str,n);
	return(ret);
	}

static int clear_tou_socket_error(int fd)
{
#ifdef DEBUG_TOU_BIO
	fprintf(stderr, "clear_tou_socket_error()\n");
#endif
	return tou_clear_error(fd);
}

static int get_last_tou_socket_error(int s)
{
#ifdef DEBUG_TOU_BIO
	fprintf(stderr, "get_last_tou_socket_error()\n");
#endif
	return tou_errno(s);
}

int BIO_tou_socket_should_retry(int s, int i)
	{
	int err;
#ifdef DEBUG_TOU_BIO
	fprintf(stderr, "BIO_tou_socket_should_retry()\n");
#endif

	if ((i == 0) || (i == -1))
		{
		err=get_last_tou_socket_error(s);

#if defined(OPENSSL_SYS_WINDOWS) && 0 /* more microsoft stupidity? perhaps not? Ben 4/1/99 */
		if ((i == -1) && (err == 0))
			return(1);
#endif

		return(BIO_tou_socket_non_fatal_error(err));
		}
	return(0);
	}

int BIO_tou_socket_non_fatal_error(int err)
	{
	switch (err)
		{
#if defined(OPENSSL_SYS_WINDOWS)
# if defined(WSAEWOULDBLOCK)
	case WSAEWOULDBLOCK:
# endif

# if 0 /* This appears to always be an error */
#  if defined(WSAENOTCONN)
	case WSAENOTCONN:
#  endif
# endif
#endif

#ifdef EWOULDBLOCK
# ifdef WSAEWOULDBLOCK
#  if WSAEWOULDBLOCK != EWOULDBLOCK
	case EWOULDBLOCK:
#  endif
# else
	case EWOULDBLOCK:
# endif
#endif

#if defined(ENOTCONN)
	case ENOTCONN:
#endif

#ifdef EINTR
	case EINTR:
#endif

#ifdef EAGAIN
#if EWOULDBLOCK != EAGAIN
	case EAGAIN:
# endif
#endif

#ifdef EPROTO
	case EPROTO:
#endif

#ifdef EINPROGRESS
	case EINPROGRESS:
#endif

#ifdef EALREADY
	case EALREADY:
#endif


#ifdef DEBUG_TOU_BIO
	fprintf(stderr, "BIO_tou_socket_non_fatal_error(%d) = 1\n", err);
#endif
		return(1);
		/* break; */
	default:
		break;
		}
#ifdef DEBUG_TOU_BIO
	fprintf(stderr, "BIO_tou_socket_non_fatal_error(%d) = 0\n", err);
#endif
	return(0);
	}
