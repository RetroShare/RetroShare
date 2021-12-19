/*******************************************************************************
 * RetroShare JNI utilities                                                    *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2021  Gioacchino Mazzurco <gio@eigenlab.org>                  *
 * Copyright (C) 2021  Asociación Civil Altermundi <info@altermundi.net>       *
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

#include <system_error>
#include <cstdlib>

#include <jni/jni.hpp>

#include "util/rsmemory.h"
#include "util/cxx23retrocompat.h"


/** Store JVM pointer safely and register native methods */
extern "C" JNIEXPORT jint JNICALL JNI_OnLoad_retroshare(JavaVM* vm, void*);


/** Provide library wide JVM access with some safe measures
 * The JVM pointer is set properly by @see JNI_OnLoad_retroshare
 */
class RsJni
{
public:
	static inline JavaVM& getVM()
	{
		if(!mJvm) // [[unlikely]]
		{
			RS_FATAL( "Attempt to access JVM before JNI_OnLoad_retroshare ",
			          std::errc::bad_address );
			print_stacktrace();
			std::exit(std::to_underlying(std::errc::bad_address));
		}

		return *mJvm;
	}

	friend jint JNI_OnLoad_retroshare(JavaVM* vm, void*);

	/** Provide a comfortable way to access Android package assets like
	 * bdboot.txt from C++ */
	struct AssetHelper
	{
		static constexpr auto Name()
		{ return "org/retroshare/service/AssetHelper"; }
	};

	/** Provide a comfortable way to propagate C++ error_conditions to Java
	 * callers */
	struct ErrorConditionWrap
	{
		static constexpr auto Name()
		{ return "org/retroshare/service/ErrorConditionWrap"; }
	};

private:
	static rs_view_ptr<JavaVM> mJvm;
};


namespace jni
{
/** Provides idiomatic way of creating instances via
@code{.cpp}
  jni::Make<ErrorConditionWrap>(env, std::error_condition());
@endcode */
jni::Local<jni::Object<RsJni::ErrorConditionWrap>>
MakeAnything(
        jni::ThingToMake<RsJni::ErrorConditionWrap>, JNIEnv& env,
        const std::error_condition& ec );
}
