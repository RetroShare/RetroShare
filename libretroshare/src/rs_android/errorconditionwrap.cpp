/*******************************************************************************
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

#include "rs_android/rsjni.hpp"

namespace jni
{
Local<Object<RsJni::ErrorConditionWrap>> MakeAnything(
        ThingToMake<RsJni::ErrorConditionWrap>, JNIEnv& env,
        const std::error_condition& ec )
{
	auto& clazz = jni::Class<RsJni::ErrorConditionWrap>::Singleton(env);

	static auto method =
	        clazz.GetConstructor<jni::jint, jni::String, jni::String>(env);

	jni::jint value = ec.value();
	auto message = jni::Make<jni::String>(env, ec.message());
	auto category = jni::Make<jni::String>(env, ec.category().name());

	return clazz.New(env, method, value, message, category);
}
}
