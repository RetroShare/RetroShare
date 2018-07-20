/*******************************************************************************
 * libretroshare/src/plugins: dlfcn_win32.h                                    *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2007 Ramiro Polla                                                 *
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
#pragma once

#ifdef WINDOWS_SYS
#ifndef DLFCN_H
#define DLFCN_H

/* POSIX says these are implementation-defined.
 * To simplify use with Windows API, we treat them the same way.
 */

#define RTLD_LAZY   0
#define RTLD_NOW    0

#define RTLD_GLOBAL (1 << 1)
#define RTLD_LOCAL  (1 << 2)

/* These two were added in The Open Group Base Specifications Issue 6.
 * Note: All other RTLD_* flags in any dlfcn.h are not standard compliant.
 */

#define RTLD_DEFAULT    0
#define RTLD_NEXT       0

void *dlopen ( const char *file, int mode );
int   dlclose( void *handle );
void *dlsym  ( void *handle, const char *name );
char *dlerror( void );

#endif /* DLFCN_H */
#endif /* WINDOWS_SYS */
