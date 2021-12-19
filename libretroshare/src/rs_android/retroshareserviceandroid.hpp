/*
 * RetroShare Service Android
 * Copyright (C) 2016-2021  Gioacchino Mazzurco <gio@eigenlab.org>
 * Copyright (C) 2021  Asociación Civil Altermundi <info@altermundi.net>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-FileCopyrightText: Retroshare Team <contact@retroshare.cc>
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */
#pragma once

#include <system_error>
#include <memory>

#include <jni/jni.hpp>

#include "rs_android/rsjni.hpp"
#include "rs_android/androidcoutcerrcatcher.hpp"

#include "util/stacktrace.h"

/** Provide native methods that are registered into corresponding Java class
 *  to start/stop RetroShare with reasonable comfort on Android platform */
struct RetroShareServiceAndroid
{
	static constexpr auto Name()
	{ return "org/retroshare/service/RetroShareServiceAndroid"; }

	using ErrorConditionWrap = RsJni::ErrorConditionWrap;

	/**
	 * Called from RetroShareServiceAndroid Java to init libretroshare
	 * @param[in] env the usual JNI parafernalia
	 * @param[in] jclass the usual JNI parafernalia
	 * @param[in] jsonApiPort port on which JSON API server will listen
	 * @param[in] jsonApiBindAddress binding address of the JSON API server
	 * @note Yeah you read it well we use a full 32 bit signed integer for JSON
	 * API port. This is because Java lack even the minimum decency to implement
	 * unsigned integral types so we need to wrap the port (16 bit unsigned
	 * integer everywhere reasonable) into a full integer and then check at
	 * runtime the value.
	 */
	static jni::Local<jni::Object<ErrorConditionWrap>> start(
	        JNIEnv& env, jni::Class<RetroShareServiceAndroid>& jclass,
	       jni::jint jsonApiPort, const jni::String& jsonApiBindAddress );

	/**
	 * Called from RetroShareServiceAndroid Java to shutdown libretroshare
	 * @param[in] env the usual JNI parafernalia
	 * @param[in] jclass the usual JNI parafernalia
	 */
	static jni::Local<jni::Object<ErrorConditionWrap>> stop(
	        JNIEnv& env, jni::Class<RetroShareServiceAndroid>& );

	struct Context
	{
		/// JNI parafernalia
		static constexpr auto Name() { return "android/content/Context"; }
	};

	/// Return RetroShare Service Android Context
	static jni::Local<jni::Object<Context>> getAndroidContext(JNIEnv& env);

private:
	/** Doesn't involve complex liftime handling stuff better let the runtime
	 * handle costruction (ASAP)/destruction for us */
	static CrashStackTrace CrashStackTrace;

	/** Involve threads, file descriptors etc. better handle lifetime
	 * explicitely */
	static std::unique_ptr<AndroidCoutCerrCatcher> sAndroidCoutCerrCatcher;
};
