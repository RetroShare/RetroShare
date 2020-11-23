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

#ifdef __ANDROID__
#	include "util/rsdebug.h"
#endif

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
        bool demangle = true, FILE *out = stderr, unsigned int maxFrames = 63 );


#if defined(__linux__) && defined(__GLIBC__)

#include <execinfo.h>
#include <cxxabi.h>

static inline void print_stacktrace(
        bool demangle, FILE* out, unsigned int maxFrames )
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
#elif defined(__ANDROID__) // defined(__linux__) && defined(__GLIBC__)

/* Inspired by the solution proposed by Louis Semprini  on this thread
 * https://stackoverflow.com/questions/8115192/android-ndk-getting-the-backtrace/35586148
 */

#include <unwind.h>
#include <dlfcn.h>
#include <cxxabi.h>

struct RsAndroidBacktraceState
{
	void** current;
	void** end;
};

static inline _Unwind_Reason_Code android_unwind_callback(
        struct _Unwind_Context* context, void* arg )
{
	RsAndroidBacktraceState* state = static_cast<RsAndroidBacktraceState*>(arg);
	uintptr_t pc = _Unwind_GetIP(context);
	if(pc)
	{
		if (state->current == state->end) return _URC_END_OF_STACK;

		*state->current++ = reinterpret_cast<void*>(pc);
	}
	return _URC_NO_REASON;
}

static inline void print_stacktrace(
        bool demangle, FILE* /*out*/, unsigned int /*maxFrames*/)
{
	constexpr int max = 5000;
	void* buffer[max];

	RsAndroidBacktraceState state;
	state.current = buffer;
	state.end = buffer + max;

	_Unwind_Backtrace(android_unwind_callback, &state);

	RsDbg() << std::endl << std::endl
	        << 0 << " " << buffer[0] << " " << __PRETTY_FUNCTION__ << std::endl;

	// Skip first frame which is print_stacktrace
	int count = static_cast<int>(state.current - buffer);
	for(int idx = 1; idx < count; ++idx)
	{
		const void* addr = buffer[idx];

		/* Ignore null addresses.
		 * They sometimes happen when using _Unwind_Backtrace()
		 * with compiler optimizations, when the Link Register is overwritten by
		 * the inner stack frames. */
		if(!addr) continue;

		/* Ignore duplicate addresses.
		 * They sometimes happen when using _Unwind_Backtrace() with compiler
		 * optimizations. */
		if(addr == buffer[idx-1]) continue;

		Dl_info info;
		if( !(dladdr(addr, &info) && info.dli_sname) )
		{
			RsDbg() << idx << " " << addr << " " << info.dli_fname
			        << " symbol not found" << std::endl;
			continue;
		}

		if(demangle)
		{
			int status = 0;
			char* demangled = __cxxabiv1::__cxa_demangle(
			            info.dli_sname, nullptr, nullptr, &status );

			if(demangled && (status == 0))
				RsDbg() << idx << " " << addr << " " << demangled << std::endl;
			else
				RsDbg() << idx << " " << addr << " "
				        << (info.dli_sname ? info.dli_sname : info.dli_fname)
				        << " __cxa_demangle failed with: " << status
				        << std::endl;

			free(demangled);
		}
		else RsDbg() << idx << " " << addr << " "
		             << (info.dli_sname ? info.dli_sname : info.dli_fname)
		             << std::endl;
	}

	RsDbg() << std::endl << std::endl;
}

#else // defined(__linux__) && defined(__GLIBC__)

static inline void print_stacktrace(
        bool /*demangle*/, FILE* out, unsigned int /*max_frames*/ )
{
	/** Notify the user which signal was caught. We use printf, because this
	 * is the most basic output function. Once you get a crash, it is
	 * possible that more complex output systems like streams and the like
	 * may be corrupted. So we make the most basic call possible to the
	 * lowest level, most standard print function. */
	fprintf(out, "print_stacktrace Not implemented yet for this platform\n");
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

	[[ noreturn ]]
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

#ifndef __ANDROID__
		/** Notify the user which signal was caught. We use printf, because this
		 * is the most basic output function. Once you get a crash, it is
		 * possible that more complex output systems like streams and the like
		 * may be corrupted. So we make the most basic call possible to the
		 * lowest level, most standard print function. */
		if(name)
			fprintf(stderr, "Caught signal %d (%s)\n", signum, name);
		else
			fprintf(stderr, "Caught signal %d\n", signum);
#else // ndef __ANDROID__
		/** On Android the best we can to is to rely on RS debug utils */
		RsFatal() << __PRETTY_FUNCTION__ << " Caught signal " << signum << " "
		          << (name ? name : "") << std::endl;
#endif

		print_stacktrace(false);

		exit(-signum);
	}
};

