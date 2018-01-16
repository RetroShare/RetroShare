#pragma once
/*
 * RetroShare Serialiser.
 * Copyright (C) 2016-2018  Gioacchino Mazzurco <gio@eigenlab.org>
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
 */

#include "serialiser/rsserializer.h"

/** @brief Minimal ancestor for all serializable structs in RetroShare
 * If you want your struct to be easly serializable you should inherit from this
 * struct.
 */
struct RsSerializable
{
	/** Register struct members to serialize in this method taking advantage of
	 * the helper macros
	 * @see RS_REGISTER_SERIAL_MEMBER(I)
	 * @see RS_REGISTER_SERIAL_MEMBER_TYPED(I, T)
	 * @see RS_REGISTER_SERIALIZABLE_TYPE(T)
	 */
	virtual void serial_process(RsGenericSerializer::SerializeJob j,
	                            RsGenericSerializer::SerializeContext& ctx) = 0;
};


/** @def RS_REGISTER_SERIAL_MEMBER(I)
 * Use this macro to register the members of `YourSerializable` for serial
 * processing inside `YourSerializable::serial_process(j, ctx)`
 *
 * Inspired by http://stackoverflow.com/a/39345864
 */
#define RS_REGISTER_SERIAL_MEMBER(I) \
	do { RsTypeSerializer::serial_process(j, ctx, I, #I); } while(0)


/** @def RS_REGISTER_SERIAL_MEMBER_TYPED(I, T)
 * This macro usage is similar to @see RS_REGISTER_SERIAL_MEMBER(I) but it
 * permit to force serialization/deserialization type, it is expecially useful
 * with enum class members or RsTlvItem derivative members, be very careful with
 * the type you pass, as reinterpret_cast on a reference is used that is
 * expecially permissive so you can shot your feet if not carefull enough.
 *
 * If you are using this with an RsSerializable derivative (so passing
 * RsSerializable as T) consider to register your item type with
 * @see RS_REGISTER_SERIALIZABLE_TYPE(T) in
 * association with @see RS_REGISTER_SERIAL_MEMBER(I) that rely on template
 * function generation, as in this particular case
 * RS_REGISTER_SERIAL_MEMBER_TYPED(I, T) would cause the serial code rely on
 * C++ dynamic dispatching that may have a noticeable impact on runtime
 * performances.
 */
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#define RS_REGISTER_SERIAL_MEMBER_TYPED(I, T) do {\
	RsTypeSerializer::serial_process<T>(j, ctx, reinterpret_cast<T&>(I), #I);\
	} while(0)
#pragma GCC diagnostic pop


/** @def RS_REGISTER_SERIALIZABLE_TYPE(T)
 * Use this macro into `youritem.cc` only if you need to process members of
 * subtypes of RsSerializable.
 *
 * The usage of this macro is strictly needed only in some cases, for example if
 * you are registering for serialization a member of a container that contains
 * items of subclasses of RsSerializable like
 * `std::map<uint64_t, RsChatMsgItem>`
 *
 * @code{.cpp}
struct PrivateOugoingMapItem : RsChatItem
{
	PrivateOugoingMapItem() : RsChatItem(RS_PKT_SUBTYPE_OUTGOING_MAP) {}

	void serial_process( RsGenericSerializer::SerializeJob j,
						 RsGenericSerializer::SerializeContext& ctx );

	std::map<uint64_t, RsChatMsgItem> store;
};

RS_REGISTER_SERIALIZABLE_TYPE(RsChatMsgItem)

void PrivateOugoingMapItem::serial_process(
		RsGenericSerializer::SerializeJob j,
		RsGenericSerializer::SerializeContext& ctx )
{
	// store is of type
	RS_REGISTER_SERIAL_MEMBER(store);
}
 * @endcode
 *
 * If you use this macro with a lot of different item types this can cause the
 * generated binary grow in size, consider the usage of
 * @see RS_REGISTER_SERIAL_MEMBER_TYPED(I, T) passing RsSerializable as type in
 * that case.
 */
#define RS_REGISTER_SERIALIZABLE_TYPE_DEF(T) template<> /*static*/\
	void RsTypeSerializer::serial_process<T>( \
	        RsGenericSerializer::SerializeJob j,\
	        RsGenericSerializer::SerializeContext& ctx, T& item,\
	        const std::string& objName ) \
{ \
	RsTypeSerializer::serial_process<RsSerializable>( j, \
	        ctx, static_cast<RsSerializable&>(item), objName ); \
}


/** @def RS_REGISTER_SERIALIZABLE_TYPE_DECL(T)
 * The usage of this macro into your header file is needed only in case you
 * needed @see RS_REGISTER_SERIALIZABLE_TYPE_DEF(T) in your definitions file,
 * but it was not enough and the compiler kept complanining about undefined
 * references to serialize, deserialize, serial_size, print_data, to_JSON,
 * from_JSON for your RsSerializable derrived type.
 *
 * One example of such case is RsIdentityUsage that is declared in
 * retroshare/rsidentity.h but defined in services/p3idservice.cc, also if
 * RS_REGISTER_SERIALIZABLE_TYPE_DEF(RsIdentityUsage) was used in p3idservice.cc
 * for some reason it was not enough for the compiler to see it so
 * RS_REGISTER_SERIALIZABLE_TYPE_DECL(RsIdentityUsage) has been added in
 * rsidentity.h too and now the compiler is happy.
 */
#define RS_REGISTER_SERIALIZABLE_TYPE_DECL(T) template<> /*static*/\
	void RsTypeSerializer::serial_process<T>( \
	        RsGenericSerializer::SerializeJob j,\
	        RsGenericSerializer::SerializeContext& ctx, T& item,\
	        const std::string& /*objName*/ );
