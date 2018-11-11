/*******************************************************************************
 * libretroshare                                                               *
 *                                                                             *
 * Copyright (C) 2016-2018 Gioacchino Mazzurco <gio@eigenlab.org>              *
 * Copyright (C) 2008 Timo Bingmann http://idlebox.net/                        *
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

#include <cstdio>
#include <csignal>
#include <cstdlib>
#include <stdio.h>

#if defined(__linux__) && defined(__GLIBC__)

#include <execinfo.h>
#include <cxxabi.h>

/**
 * @brief Print a backtrace to FILE* out.
 * @param[in] demangle true to demangle C++ symbols requires malloc working, in
 *	some patological cases like a SIGSEGV received during a malloc this would
 *	cause deadlock so pass false if you may be in such situation (like in a
 *	SIGSEGV handler )
 * @param[in] out output file
 * @param[in] maxFrames maximum number of stack frames you want to bu printed
 */
static inline void print_stacktrace(
        bool demangle = true, FILE *out = stderr, unsigned int maxFrames = 63 )
{
	if(!out)
	{
		fprintf(stderr, "print_stacktrace invalid output file!\n");
		return;
	}

	fprintf(out, "stack trace:\n");

	// storage array for stack trace address data
	void* addrlist[maxFrames+1];

	// retrieve current stack addresses
	int addrlen = backtrace(addrlist, sizeof(addrlist) / sizeof(void*));

	if (addrlen == 0)
	{
		fprintf(out, "  <empty, possibly corrupt>\n");
		return;
	}

	if(!demangle)
	{
		int outFd = fileno(out);
		if(outFd < 0)
		{
			fprintf(stderr, "print_stacktrace invalid output file descriptor!\n");
			return;
		}

		backtrace_symbols_fd(addrlist, addrlen, outFd);
		return;
	}

	// resolve addresses into strings containing "filename(function+address)",
	// this array must be free()-ed
	char** symbollist = backtrace_symbols(addrlist, addrlen);

	// allocate string which will be filled with the demangled function name
	size_t funcnamesize = 256;
	char* funcname = (char*)malloc(funcnamesize);

	// iterate over the returned symbol lines. skip the first, it is the
	// address of this function.
	for (int i = 1; i < addrlen; i++)
	{
		char *begin_name = 0, *begin_offset = 0, *end_offset = 0;

		/* find parentheses and +address offset surrounding the mangled
		 * name: ./module(function+0x15c) [0x8048a6d] */
		for (char *p = symbollist[i]; *p; ++p)
		{
			if (*p == '(') begin_name = p;
			else if (*p == '+') begin_offset = p;
			else if (*p == ')' && begin_offset)
			{
				end_offset = p;
				break;
			}
		}

		if ( begin_name && begin_offset && end_offset
		     && begin_name < begin_offset )
		{
			*begin_name++ = '\0';
			*begin_offset++ = '\0';
			*end_offset = '\0';

			// mangled name is now in [begin_name, begin_offset) and caller
			// offset in [begin_offset, end_offset). now apply
			// __cxa_demangle():

			int status;
			char* ret = abi::__cxa_demangle(
			            begin_name, funcname, &funcnamesize, &status );
			if (status == 0)
			{
				funcname = ret; // use possibly realloc()-ed string
				fprintf( out, "  %s : %s+%s\n",
				         symbollist[i], funcname, begin_offset );
			}
			else
			{
				// demangling failed. Output function name as a C function with
				// no arguments.
				fprintf( out, "  %s : %s()+%s\n",
				         symbollist[i], begin_name, begin_offset );
			}
		}
		else
		{
			// couldn't parse the line? print the whole line.
			fprintf(out, "  %s\n", symbollist[i]);
		}
	}

	free(funcname);
	free(symbollist);
}

#else // defined(__linux__) && defined(__GLIBC__)
static inline void print_stacktrace(
        bool demangle = true, FILE *out = stderr, unsigned int max_frames = 63 )
{
	(void) demangle;
	(void) max_frames;

	fprintf(out, "TODO: 2016/01/01 print_stacktrace not implemented yet for WINDOWS_SYS and ANDROID\n");
}
#endif // defined(__linux__) && defined(__GLIBC__)

/**
 * @brief CrashStackTrace catch crash signals and print stack trace
 * Inspired too https://oroboro.com/stack-trace-on-crash/
 */
struct CrashStackTrace
{
	CrashStackTrace()
	{
		signal(SIGABRT, &CrashStackTrace::abortHandler);
		signal(SIGSEGV, &CrashStackTrace::abortHandler);
		signal(SIGILL,  &CrashStackTrace::abortHandler);
		signal(SIGFPE,  &CrashStackTrace::abortHandler);
#ifdef SIGBUS
		signal(SIGBUS,  &CrashStackTrace::abortHandler);
#endif
	}

	static void abortHandler(int signum)
	{
		// associate each signal with a signal name string.
		const char* name = nullptr;
		switch(signum)
		{
		case SIGABRT: name = "SIGABRT";  break;
		case SIGSEGV: name = "SIGSEGV";  break;
		case SIGILL:  name = "SIGILL";   break;
		case SIGFPE:  name = "SIGFPE";   break;
#ifdef SIGBUS
		case SIGBUS:  name = "SIGBUS";   break;
#endif
		}

		/** Notify the user which signal was caught. We use printf, because this
		 * is the most basic output function. Once you get a crash, it is
		 * possible that more complex output systems like streams and the like
		 * may be corrupted. So we make the most basic call possible to the
		 * lowest level, most standard print function. */
		if(name)
			fprintf(stderr, "Caught signal %d (%s)\n", signum, name);
		else
			fprintf(stderr, "Caught signal %d\n", signum);

		print_stacktrace(false);

		exit(-signum);
	}
};

