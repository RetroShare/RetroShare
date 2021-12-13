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

#include "rs_android/rsjni.hpp"
#include "rs_android/retroshareserviceandroid.hpp"

rs_view_ptr<JavaVM> RsJni::mJvm = nullptr;


extern "C" JNIEXPORT jint JNICALL JNI_OnLoad_retroshare(JavaVM* vm, void*)
{
	RS_DBG(vm);

	RsJni::mJvm = vm;

	jni::JNIEnv& env { jni::GetEnv(*vm) };

	/** Ensure singleton refereces to our own Java classes are inizialized here
	 * because default Java class loader which is the one accessible by native
	 * threads which is not main even if attached, is not capable to find them.
	 * https://stackoverflow.com/questions/20752352/classnotfoundexception-when-finding-a-class-in-jni-background-thread
	 * https://groups.google.com/g/android-ndk/c/2gkr1mXKn_E */
	jni::Class<RsJni::AssetHelper>::Singleton(env);
	jni::Class<RsJni::ErrorConditionWrap>::Singleton(env);

	jni::RegisterNatives(
	            env, *jni::Class<RetroShareServiceAndroid>::Singleton(env),
	            jni::MakeNativeMethod<
	                decltype(&RetroShareServiceAndroid::start),
	                &RetroShareServiceAndroid::start >("nativeStart"),
	            jni::MakeNativeMethod<
	                decltype(&RetroShareServiceAndroid::stop),
	                &RetroShareServiceAndroid::stop >("nativeStop")
	            );

	return jni::Unwrap(jni::jni_version_1_2);
}

#ifdef RS_LIBRETROSHARE_EXPORT_JNI_ONLOAD
/** If libretroshare is linked statically to other components which already
 * export JNI_OnLoad then a symbol clash may happen
 * if RS_LIBRETROSHARE_EXPORT_JNI_ONLOAD is defined.
 * @see JNI_OnLoad_retroshare should instead be called from the exported
 * JNI_OnLoad */
extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* _reserved)
{
	RS_DBG(vm);
	return JNI_OnLoad_retroshare(vm, _reserved);
}
#endif // def RS_LIBRETROSHARE_EXPORT_JNI_ONLOAD
