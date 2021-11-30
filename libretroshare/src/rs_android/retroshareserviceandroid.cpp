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

#include <limits>
#include <cstdint>

#include "util/stacktrace.h"
#include "retroshare/rsinit.h"
#include "retroshare/rsiface.h"
#include "util/rsdebug.h"

#include "rs_android/retroshareserviceandroid.hpp"
#include "rs_android/rsjni.hpp"


/*static*/ std::unique_ptr<AndroidCoutCerrCatcher>
RetroShareServiceAndroid::sAndroidCoutCerrCatcher = nullptr;

using ErrorConditionWrap = RsJni::ErrorConditionWrap;

/*static*/ jni::Local<jni::Object<ErrorConditionWrap>>
RetroShareServiceAndroid::start(
        JNIEnv& env, jni::Class<RetroShareServiceAndroid>&,
        jni::jint jsonApiPort, const jni::String& jsonApiBindAddress )
{
	if(jsonApiPort < 0 || jsonApiPort > std::numeric_limits<uint16_t>::max())
	{
		RS_ERR("Got invalid JSON API port: ", jsonApiPort);
		return jni::Make<ErrorConditionWrap>(env, std::errc::invalid_argument);
	}

	RsInfo() << "\n" <<
	            "+================================================================+\n"
	            "|     o---o                                             o        |\n"
	            "|      \\ /       - Retroshare Service Android -        / \\       |\n"
	            "|       o                                             o---o      |\n"
	            "+================================================================+"
	         << std::endl << std::endl;

	sAndroidCoutCerrCatcher = std::make_unique<AndroidCoutCerrCatcher>();

	RsInit::InitRsConfig();
	RsControl::earlyInitNotificationSystem();

	RsConfigOptions conf;
	conf.jsonApiPort = static_cast<uint16_t>(jsonApiPort);
	conf.jsonApiBindAddress = jni::Make<std::string>(env, jsonApiBindAddress);

	// Dirty workaround plugins not supported on Android ATM
	conf.main_executable_path = " ";

	int initResult = RsInit::InitRetroShare(conf);
	if(initResult != RS_INIT_OK)
	{
		RS_ERR("Retroshare core initalization failed with: ", initResult);
		return jni::Make<ErrorConditionWrap>(env, std::errc::no_child_process);
	}

	return jni::Make<ErrorConditionWrap>(env, std::error_condition());
}

jni::Local<jni::Object<ErrorConditionWrap>> RetroShareServiceAndroid::stop(
        JNIEnv& env, jni::Class<RetroShareServiceAndroid>& )
{
	if(RsControl::instance()->isReady())
	{
		RsControl::instance()->rsGlobalShutDown();
		return jni::Make<ErrorConditionWrap>(env, std::error_condition());
	}

	sAndroidCoutCerrCatcher.reset();

	return jni::Make<ErrorConditionWrap>(env, std::errc::no_such_process);
}

jni::Local<jni::Object<RetroShareServiceAndroid::Context> >
RetroShareServiceAndroid::getAndroidContext(JNIEnv& env)
{
	auto& clazz = jni::Class<RetroShareServiceAndroid>::Singleton(env);
	static auto method =
	        clazz.GetStaticMethod<jni::Object<RetroShareServiceAndroid::Context>()>(
	            env, "getServiceContext" );
	return clazz.Call(env, method);
}
