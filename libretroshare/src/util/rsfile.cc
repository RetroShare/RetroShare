/*******************************************************************************
 * libretroshare/src/util: rsfile.cc                                           *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2021 Retroshare Team <retroshare.project@gmail.com>           *
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

#include "util/rsfile.h"

#ifdef WINDOWS_SYS
#include <wtypes.h>
#include <io.h>
#include <namedpipeapi.h>
#include <errno.h>
#else
#include <fcntl.h>
#endif

int RsFileUtil::set_fd_nonblock(int fd)
{
    int ret = 0;

/******************* OS SPECIFIC PART ******************/
#ifdef WINDOWS_SYS
    DWORD mode = PIPE_NOWAIT;
    WINBOOL result = SetNamedPipeHandleState((HANDLE) _get_osfhandle(fd), &mode, nullptr, nullptr);

    if (!result) {
        ret = -1;
    }
#else // ie UNIX
    int flags = fcntl(fd, F_GETFL);
    ret = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
#endif

    return ret;
}

ssize_t RsFileUtil::rs_getline(char **lineptr, size_t *n, FILE *stream)
{
/******************* OS SPECIFIC PART ******************/
#ifdef WINDOWS_SYS
    if (lineptr == nullptr || n == nullptr || stream == nullptr) {
        errno = EINVAL;
        return -1;
    }

    if (*lineptr == nullptr || *n < 1) {
        *n = BUFSIZ;
        *lineptr = (char*) malloc(*n);
        if (*lineptr == nullptr) {
            *n = 0;
            return -1;
        }
    }

    char *ptr = *lineptr;
    while (true) {
        int c = fgetc(stream);
        if (c == -1) {
            if (feof(stream)) {
                *ptr = '\0';
                return (ssize_t) (ptr - *lineptr);
            }
            return -1;
        }

        *ptr = c;
        ++ptr;

        if (c == '\n') {
            *ptr = '\0';
            return ptr - *lineptr;
        }
        if (ptr + 2 >= *lineptr + *n) {
            size_t new_size = *n * 2;
            ssize_t diff = ptr - *lineptr;

            char *new_lineptr = (char*) realloc(*lineptr, new_size);
            if (new_lineptr == nullptr) {
                return -1;
            }

            *lineptr = new_lineptr;
            *n = new_size;
            ptr = new_lineptr + diff;
        }
    }
#else // ie UNIX
    return getline(lineptr, n, stream);
#endif
}
