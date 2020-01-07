/*******************************************************************************
 * libretroshare/src/util: rserrno.cc                                          *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2019  Gioacchino Mazzurco <gio@eigenlab.org>                  *
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

#include <cerrno>

#define RS_INTERNAL_ERRNO_CASE(e) case e: return #e

const char* rsErrnoName(int err)
{
	switch (err)
	{
	RS_INTERNAL_ERRNO_CASE(EINVAL);
	RS_INTERNAL_ERRNO_CASE(EBUSY);
	RS_INTERNAL_ERRNO_CASE(EAGAIN);
	RS_INTERNAL_ERRNO_CASE(EDEADLK);
	RS_INTERNAL_ERRNO_CASE(EPERM);
	RS_INTERNAL_ERRNO_CASE(EBADF);
	RS_INTERNAL_ERRNO_CASE(EFAULT);
	RS_INTERNAL_ERRNO_CASE(ENOTSOCK);
	RS_INTERNAL_ERRNO_CASE(EISCONN);
	RS_INTERNAL_ERRNO_CASE(ECONNREFUSED);
	RS_INTERNAL_ERRNO_CASE(ETIMEDOUT);
	RS_INTERNAL_ERRNO_CASE(ENETUNREACH);
	RS_INTERNAL_ERRNO_CASE(EADDRINUSE);
	RS_INTERNAL_ERRNO_CASE(EINPROGRESS);
	RS_INTERNAL_ERRNO_CASE(EALREADY);
	RS_INTERNAL_ERRNO_CASE(ENOTCONN);
	RS_INTERNAL_ERRNO_CASE(EPIPE);
	RS_INTERNAL_ERRNO_CASE(ECONNRESET);
	RS_INTERNAL_ERRNO_CASE(EHOSTUNREACH);
	RS_INTERNAL_ERRNO_CASE(EADDRNOTAVAIL);
	}

	return "rsErrnoName UNKNOWN ERROR CODE";
}
