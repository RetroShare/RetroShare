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

#include <type_traits>

#include "serialiser/rsserializer.h"


/** @brief Minimal ancestor for all serializable structs in RetroShare.
 * If you want your struct to be easly serializable you should inherit from this
 * struct.
 * If you want your struct to be serializable as part of a container like an
 * `std::vector<T>` @see RS_REGISTER_SERIALIZABLE_TYPE_DEF(T) and
 * @see RS_REGISTER_SERIALIZABLE_TYPE_DECL(T)
 */
struct RsSerializable
{
	/** Register struct members to serialize in this method taking advantage of
	 * the helper macro @see RS_SERIAL_PROCESS(I)
	 */
	virtual void serial_process(RsGenericSerializer::SerializeJob j,
	                            RsGenericSerializer::SerializeContext& ctx) = 0;
};


/** @def RS_SERIAL_PROCESS(I)
 * Use this macro to register the members of `YourSerializable` for serial
 * processing inside `YourSerializable::serial_process(j, ctx)`
 *
 * Pay special attention for member of enum type which must be declared
 * specifying the underlying type otherwise the serialization format may differ
 * in an uncompatible way depending on the compiler/platform.
 *
 * If your member is a derivative of RsSerializable in some cases it may be
 * convenient also to register your item type with
 * @see RS_REGISTER_SERIALIZABLE_TYPE_DEF(T).
 *
 * Inspired by http://stackoverflow.com/a/39345864
 */
#define RS_SERIAL_PROCESS(I) do { \
	RsTypeSerializer::serial_process(j, ctx, __priv_to_RS_S_TYPE(I), #I ); \
	} while(0)


/** @def RS_REGISTER_SERIALIZABLE_TYPE_DEF(T)
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

RS_REGISTER_SERIALIZABLE_TYPE_DEF(RsChatMsgItem)

void PrivateOugoingMapItem::serial_process(
		RsGenericSerializer::SerializeJob j,
		RsGenericSerializer::SerializeContext& ctx )
{
	RS_SERIAL_PROCESS(store);
}
 * @endcode
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
 * The usage of this macro into your header file may be needed only in case you
 * needed @see RS_REGISTER_SERIALIZABLE_TYPE_DEF(T) in your definitions file,
 * but it was not enough and the compiler kept complanining about undefined
 * references to serialize, deserialize, serial_size, print_data, to_JSON,
 * from_JSON for your RsSerializable derived type.
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
	        const std::string& objName );


//============================================================================//
// Private type conversion helpers                                            //
//============================================================================//

/**
 * @brief Privates type conversion helpers for RS_SERIAL_PROCESS.
 * @private DO NOT use explicitely outside this file.
 * Used to cast enums and derived type to matching types supported by
 * RsTypeSerializer::serial_process(...) templated methods.
 */
template<typename T, typename = typename std::enable_if<std::is_enum<T>::value>::type>
inline typename std::underlying_type<T>::type& __priv_to_RS_S_TYPE(T& i)
{
	return reinterpret_cast<typename std::underlying_type<T>::type&>(i);
}

template<typename T, typename = typename std::enable_if<std::is_base_of<RsSerializable, T>::value>::type>
inline RsSerializable& __priv_to_RS_S_TYPE(T& i)
{
	return static_cast<RsSerializable&>(i);
}

struct RsTlvItem;
template<typename T, typename = typename std::enable_if<std::is_base_of<RsTlvItem, T>::value>::type>
inline RsTlvItem& __priv_to_RS_S_TYPE(T& i)
{
	return static_cast<RsTlvItem&>(i);
}

template<typename T, typename = typename std::enable_if<!(std::is_enum<T>::value||std::is_base_of<RsTlvItem, T>::value||std::is_base_of<RsSerializable, T>::value)>::type>
inline T& __priv_to_RS_S_TYPE(T& i)
{
	return i;
}
