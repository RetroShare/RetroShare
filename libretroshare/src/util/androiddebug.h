/*******************************************************************************
 * libretroshare/src/util: androiddebug.h                                      *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2016  Gioacchino Mazzurco <gio@eigenlab.org>                  *
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

// Inspired by: https://codelab.wordpress.com/2014/11/03/how-to-use-standard-output-streams-for-logging-in-android-apps/

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>           // for O_NONBLOCK
#include <android/log.h>
#include <atomic>
#include <string>

/**
 * On Android stdout and stderr of native code is discarded, instancing this
 * class at the beginning of the main of your program to get them (stdout and
 * stderr) on logcat output.
 */
class AndroidStdIOCatcher
{
public:
    AndroidStdIOCatcher(const std::string& dTag = "UnseenP2P",
	                    android_LogPriority stdout_pri = ANDROID_LOG_INFO,
	                    android_LogPriority stderr_pri = ANDROID_LOG_ERROR) :
	    tag(dTag), cout_pri(stdout_pri), cerr_pri(stderr_pri), should_stop(false)
	{
		// make stdout line-buffered
		//setvbuf(stdout, 0, _IOLBF, 0);

		// make stdout and stderr unbuffered
		setvbuf(stdout, 0, _IONBF, 0);
		setvbuf(stderr, 0, _IONBF, 0);

		// create the pipes and redirect stdout and stderr
		pipe2(pout_fd, O_NONBLOCK);
		dup2(pout_fd[1], STDOUT_FILENO);

		pipe2(perr_fd, O_NONBLOCK);
		dup2(perr_fd[1], STDERR_FILENO);

		// spawn the logging thread
		pthread_create(&thr, 0, thread_func, this);
		pthread_detach(thr);
	}

	~AndroidStdIOCatcher()
	{
		should_stop = true;
		pthread_join(thr, NULL);
	}

private:
	const std::string tag;
	const android_LogPriority cout_pri;
	const android_LogPriority cerr_pri;

	int pout_fd[2];
	int perr_fd[2];
	pthread_t thr;
	std::atomic<bool> should_stop;

	static void *thread_func(void* instance)
	{
        __android_log_write(ANDROID_LOG_INFO, "UnseenP2P", "Android debugging start");

		AndroidStdIOCatcher &i = *static_cast<AndroidStdIOCatcher*>(instance);

		std::string out_buf;
		std::string err_buf;

		while (!i.should_stop)
		{
			for(char c; read(i.pout_fd[0], &c, 1) == 1;)
			{
				out_buf += c;
				if(c == '\n')
				{
					__android_log_write(i.cout_pri, i.tag.c_str(), out_buf.c_str());
					out_buf.clear();
				}
			}

			for(char c; read(i.perr_fd[0], &c, 1) == 1;)
			{
				err_buf += c;
				if(c == '\n')
				{
					__android_log_write(i.cerr_pri, i.tag.c_str(), err_buf.c_str());
					err_buf.clear();
				}
			}

			usleep(10000);
		}

        __android_log_write(ANDROID_LOG_INFO, "UnseenP2P", "Android debugging stop");

		return NULL;
	}
};

