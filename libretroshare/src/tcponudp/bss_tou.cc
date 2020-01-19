/*******************************************************************************
 * libretroshare/src/tcponudp: bss_tou.c                                       *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 1995-1998  Eric Young <eay@cryptsoft.com>                     *
 * Copyright (C) 2004-2006  Robert Fernie <retroshare@lunamutt.com>            *
 * Copyright (C) 2018-2019  Gioacchino Mazzurco <gio@eigenlab.org>             *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Lesser General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Lesser General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Lesser General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/

#ifndef OPENSSL_NO_SOCK

#include "bio_tou.h"

#include <stdio.h>
#include <errno.h>
#include <string.h> /* for strlen() */

#define USE_SOCKETS
#include <openssl/bio.h>

#include "tou.h"
#include "util/rsdebug.h"

RS_SET_CONTEXT_DEBUG_LEVEL(0);

static int tou_socket_write(BIO *h, const char *buf, int num);
static int tou_socket_read(BIO *h, char *buf, int size);
static int tou_socket_puts(BIO *h, const char *str);
static long tou_socket_ctrl(BIO *h, int cmd, long arg1, void *arg2);
static int tou_socket_new(BIO *h);
static int tou_socket_free(BIO *data);
static int get_last_tou_socket_error(int s);
static int clear_tou_socket_error(int s);

#if OPENSSL_VERSION_NUMBER < 0x10100000L || defined(LIBRESSL_VERSION_NUMBER)

static int  BIO_get_init(BIO *a) { return a->init; }

#if (defined(LIBRESSL_VERSION_NUMBER) && LIBRESSL_VERSION_NUMBER < 0x20700000) \
	|| (!defined(LIBRESSL_VERSION_NUMBER) && OPENSSL_VERSION_NUMBER < 0x10100000L)

static int  BIO_get_shutdown(BIO *a) { return a->shutdown; }
static void BIO_set_init(BIO *a,int i) { a->init=i; }
static void BIO_set_data(BIO *a,void *p) { a->ptr = p; }
long (*BIO_meth_get_ctrl(const BIO_METHOD* biom)) (BIO*, int, long, void*)
{ return biom->ctrl; }

#endif /* (defined(LIBRESSL_VERSION_NUMBER) && LIBRESSL_VERSION_NUMBER < 0x20700000) \
|| (!defined(LIBRESSL_VERSION_NUMBER) && OPENSSL_VERSION_NUMBER < 0x10100000L) */

static BIO_METHOD methods_tou_sockp =
{
    BIO_TYPE_TOU_SOCKET,
    "tou_socket",
    tou_socket_write,
    tou_socket_read,
    tou_socket_puts,
    nullptr, // bgets
    tou_socket_ctrl,
    tou_socket_new,
    tou_socket_free,
    nullptr, // callback_ctrl
};

BIO_METHOD* BIO_s_tou_socket(void)
{
	Dbg2() << __PRETTY_FUNCTION__ << std::endl;
	return(&methods_tou_sockp);
}

#else // OPENSSL_VERSION_NUMBER < 0x10100000L || defined(LIBRESSL_VERSION_NUMBER)

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

#endif // OPENSSL_VERSION_NUMBER < 0x10100000L || defined(LIBRESSL_VERSION_NUMBER)

static int tou_socket_new(BIO *bi)
{
	Dbg2() << __PRETTY_FUNCTION__ << std::endl;

    BIO_set_init(bi,0) ;
	BIO_set_data(bi, nullptr);	// sets bi->ptr
    BIO_set_flags(bi,0) ;
    BIO_set_fd(bi,0,0) ;
	return(1);
}

static int tou_socket_free(BIO *a)
{
	Dbg2() << __PRETTY_FUNCTION__ << std::endl;

	if (!a) return(0);

    if(BIO_get_shutdown(a))
		{
		if(BIO_get_init(a)) tou_close(static_cast<int>(BIO_get_fd(a, nullptr)));
        BIO_set_init(a,0) ;
        BIO_set_flags(a,0) ;
		}
	return(1);
	}
	
static int tou_socket_read(BIO *b, char *out, int outl)
{
	Dbg2() << __PRETTY_FUNCTION__ << std::endl;

	int ret=0;

	if (out)
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
	constexpr int fatalError = 0;
	constexpr int nonFatalError = 1;

	switch (err)
	{
#if defined(OPENSSL_SYS_WINDOWS)
#	if defined(WSAEWOULDBLOCK)
	case WSAEWOULDBLOCK:
#	endif // defined(WSAEWOULDBLOCK)
#endif // defined(OPENSSL_SYS_WINDOWS)

#ifdef EWOULDBLOCK
#	ifdef WSAEWOULDBLOCK
#		if WSAEWOULDBLOCK != EWOULDBLOCK
	case EWOULDBLOCK:
#		endif // WSAEWOULDBLOCK != EWOULDBLOCK
#	else // def WSAEWOULDBLOCK
	case EWOULDBLOCK:
#	endif // def WSAEWOULDBLOCK
#endif

#if defined(ENOTCONN)
	case ENOTCONN:
#endif // defined(ENOTCONN)

#ifdef EINTR
	case EINTR:
#endif // def EINTR

#ifdef EAGAIN
#	if EWOULDBLOCK != EAGAIN
	case EAGAIN:
#	endif //  EWOULDBLOCK != EAGAIN
#endif // def EAGAIN

#ifdef EPROTO
	case EPROTO:
#endif // def EPROTO

#ifdef EINPROGRESS
	case EINPROGRESS:
#endif // def EINPROGRESS

#ifdef EALREADY
	case EALREADY:
#endif // def EALREADY

		Dbg2() << __PRETTY_FUNCTION__ << " err: " << err
		       << " return nonFatalError " << nonFatalError << std::endl;
		return nonFatalError;

	default:
		break;
	}

	Dbg2() << __PRETTY_FUNCTION__ << " err: " << err << " return fatalError "
	       << fatalError << std::endl;

	return fatalError;
}

#endif // ndef OPENSSL_NO_SOCK
