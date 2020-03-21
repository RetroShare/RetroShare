/*******************************************************************************
 * libretroshare/src/serialiser: rstypeserializer.h                            *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2017       Cyril Soler <csoler@users.sourceforge.net>         *
 * Copyright (C) 2018-2020  Gioacchino Mazzurco <gio@eigenlab.org>             *
 * Copyright (C) 2020       Asociación Civil Altermundi <info@altermundi.net>  *
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

#include <typeinfo> // for typeid
#include <type_traits>
#include <cerrno>
#include <system_error>
#include <bitset>
#include <string>

#include "serialiser/rsserial.h"
#include "serialiser/rstlvbase.h"
#include "serialiser/rstlvlist.h"
#include "retroshare/rsflags.h"
#include "retroshare/rsids.h"
#include "util/rsendian.h"
#include "serialiser/rsserializer.h"
#include "serialiser/rsserializable.h"
#include "util/rsjson.h"
#include "util/rsdebug.h"
#include "util/cxx14retrocompat.h"


struct RsTypeSerializer
{
	/** Use this wrapper to serialize raw memory chunks
	 * RsTypeSerializer::RawMemoryWrapper chunkWrapper(chunk_data, chunk_size);
	 * RsTypeSerializer::serial_process(j, ctx, chunkWrapper, "chunk_data");
	 **/
	struct RawMemoryWrapper: std::pair<uint8_t*&,uint32_t&>, RsSerializable
	{
		RawMemoryWrapper(uint8_t*& p,uint32_t& s) :
		    std::pair<uint8_t*&,uint32_t&>(p,s) {}
		RawMemoryWrapper(void*& p, uint32_t& s) :
		    std::pair<uint8_t*&,uint32_t&>(*(uint8_t**)(&p),s) {}

		/// Maximum supported size 10MB
		static constexpr uint32_t MAX_SERIALIZED_CHUNK_SIZE = 10*1024*1024;

		/// @see RsSerializable
		void serial_process(
		        RsGenericSerializer::SerializeJob j,
		        RsGenericSerializer::SerializeContext& ctx ) override;
	private:
		void clear();
	};

	/// Most types are not valid sequence containers
	template<typename T, typename = void>
	struct is_sequence_container : std::false_type {};

	/// Trait to match supported strings types
	template<typename T, typename = void, typename = void>
	struct is_string : std::is_same<std::decay_t<T>, std::string> {};

	/// Integral types
	template<typename INTT>
	typename std::enable_if<std::is_integral<INTT>::value>::type
	static /*void*/ serial_process(
	        RsGenericSerializer::SerializeJob j,
	        RsGenericSerializer::SerializeContext& ctx,
	        INTT& member, const std::string& member_name )
	{
		const bool VLQ_ENCODING = !!(
		            RsSerializationFlags::INTEGER_VLQ & ctx.mFlags );

		switch(j)
		{
		case RsGenericSerializer::SIZE_ESTIMATE:
			if(VLQ_ENCODING) ctx.mOffset += VLQ_size(member);
			else ctx.mOffset += sizeof(INTT);
			break;
		case RsGenericSerializer::SERIALIZE:
		{
			if(!ctx.mOk) break;
			if(VLQ_ENCODING)
				ctx.mOk = VLQ_serialize(
				            ctx.mData, ctx.mSize, ctx.mOffset, member );
			else
			{
				ctx.mOk = ctx.mSize >= ctx.mOffset + sizeof(INTT);
				if(!ctx.mOk)
				{
					RsErr() << __PRETTY_FUNCTION__ << " Cannot serialise "
					        << typeid(INTT).name() << " "
					        << " ctx.mSize: " << ctx.mSize
					        << " ctx.mOffset: " << ctx.mOffset
					        << " sizeof(INTT): " << sizeof(INTT)
					        << std::error_condition(std::errc::no_buffer_space)
					        << std::endl;
					print_stacktrace();
					break;
				}
				INTT netorder_num = rs_endian_fix(member);
				memcpy(ctx.mData + ctx.mOffset, &netorder_num, sizeof(INTT));
				ctx.mOffset += sizeof(INTT);
			}
			break;
		}
		case RsGenericSerializer::DESERIALIZE:
			if(!ctx.mOk) break;
			if(VLQ_ENCODING) ctx.mOk = VLQ_deserialize(
			            ctx.mData, ctx.mSize, ctx.mOffset, member );
			else
			{
				ctx.mOk = ctx.mSize >= ctx.mOffset + sizeof(INTT);
				if(!ctx.mOk)
				{
					RsErr() << __PRETTY_FUNCTION__ << " Cannot deserialise "
					        << typeid(INTT).name() << " "
					        << " ctx.mSize: " << ctx.mSize
					        << " ctx.mOffset: " << ctx.mOffset
					        << " sizeof(INTT): " << sizeof(INTT)
					        << std::error_condition(std::errc::no_buffer_space)
					        << std::endl;
					print_stacktrace();
					exit(-1);
					break;
				}
				memcpy(&member, ctx.mData + ctx.mOffset, sizeof(INTT));
				member = rs_endian_fix(member);
				ctx.mOffset += sizeof(INTT);
			}
			break;
		case RsGenericSerializer::PRINT: break;
		case RsGenericSerializer::TO_JSON:
			ctx.mOk = ctx.mOk && to_JSON(member_name, member, ctx.mJson);
			break;
		case RsGenericSerializer::FROM_JSON:
			ctx.mOk &= ( ctx.mOk ||
			             !!(RsSerializationFlags::YIELDING & ctx.mFlags) )
			        && from_JSON(member_name, member, ctx.mJson);
			break;
		default: fatalUnknownSerialJob(j);
		}
	}

//============================================================================//
//                             Generic types                                  //
//============================================================================//

	template<typename T>
	typename
	std::enable_if< std::is_same<RsTlvItem,T>::value || !(
	        std::is_integral<T>::value ||
	        std::is_base_of<RsSerializable,T>::value ||
	        std::is_enum<T>::value ||
	        std::is_base_of<RsTlvItem,T>::value ||
	        std::is_same<std::error_condition,T>::value ||
	        is_sequence_container<T>::value || is_string<T>::value ) >::type
	static /*void*/ serial_process( RsGenericSerializer::SerializeJob j,
	                                RsGenericSerializer::SerializeContext& ctx,
	                                T& memberC, const std::string& member_name )
	{
		// Avoid problems with const sneaking into template paramether
		using m_t = std::remove_const_t<T>;
		m_t& member = const_cast<m_t&>(memberC);

		switch(j)
		{
		case RsGenericSerializer::SIZE_ESTIMATE:
			ctx.mOffset += serial_size(member);
			break;
		case RsGenericSerializer::DESERIALIZE:
			ctx.mOk = ctx.mOk &&
			        deserialize(ctx.mData,ctx.mSize,ctx.mOffset,member);
			break;
		case RsGenericSerializer::SERIALIZE:
			ctx.mOk = ctx.mOk &&
			        serialize(ctx.mData,ctx.mSize,ctx.mOffset,member);
			break;
		case RsGenericSerializer::PRINT:
			print_data(member_name,member);
			break;
		case RsGenericSerializer::TO_JSON:
			ctx.mOk = ctx.mOk && to_JSON(member_name, member, ctx.mJson);
			break;
		case RsGenericSerializer::FROM_JSON:
			ctx.mOk &= ( ctx.mOk ||
			             !!(ctx.mFlags & RsSerializationFlags::YIELDING) )
			        && from_JSON(member_name, member, ctx.mJson);
			break;
		default: fatalUnknownSerialJob(j);
		}
	}

//============================================================================//
//                        Generic types + type_id                             //
//============================================================================//

	/// Generic types + type_id
	template<typename T> RS_DEPRECATED
	static void serial_process(
	        RsGenericSerializer::SerializeJob j,
	        RsGenericSerializer::SerializeContext& ctx,
	        uint16_t type_id, T& member, const std::string& member_name )
	{
		switch(j)
		{
		case RsGenericSerializer::SIZE_ESTIMATE:
			ctx.mOffset += serial_size(type_id,member);
			break;
		case RsGenericSerializer::DESERIALIZE:
			ctx.mOk = ctx.mOk &&
			        deserialize(ctx.mData,ctx.mSize,ctx.mOffset,type_id,member);
			break;
		case RsGenericSerializer::SERIALIZE:
			ctx.mOk = ctx.mOk &&
			        serialize(ctx.mData,ctx.mSize,ctx.mOffset,type_id,member);
			break;
		case RsGenericSerializer::PRINT: break;
		case RsGenericSerializer::TO_JSON:
			ctx.mOk = ctx.mOk &&
			        to_JSON(member_name, type_id, member, ctx.mJson);
			break;
		case RsGenericSerializer::FROM_JSON:
			ctx.mOk &=
			        (ctx.mOk || !!(ctx.mFlags & RsSerializationFlags::YIELDING))
			        && from_JSON(member_name, type_id, member, ctx.mJson);
			break;
		default: fatalUnknownSerialJob(j);
		}
	}

//============================================================================//
//                               std::map                                     //
//============================================================================//

	/// std::map<T,U>
	template<typename T,typename U>
	static void serial_process( RsGenericSerializer::SerializeJob j,
	                            RsGenericSerializer::SerializeContext& ctx,
	                            std::map<T,U>& member,
	                            const std::string& memberName )
	{
		switch(j)
		{
		case RsGenericSerializer::SIZE_ESTIMATE: // [[falltrough]]
		case RsGenericSerializer::SERIALIZE:
		{
			uint32_t mapSize = member.size();
			RS_SERIAL_PROCESS(mapSize);

			for( auto it = member.begin();
			     ctx.mOk && it != member.end(); ++it )
			{
				RS_SERIAL_PROCESS(const_cast<T&>(it->first));
				RS_SERIAL_PROCESS(const_cast<U&>(it->second));
			}
			break;
		}
		case RsGenericSerializer::DESERIALIZE:
		{
			uint32_t mapSize = 0;
			RS_SERIAL_PROCESS(mapSize);

			for(uint32_t i=0; ctx.mOk && i<mapSize; ++i)
			{
				T t; U u;
				RS_SERIAL_PROCESS(t);
				RS_SERIAL_PROCESS(u);
				member[t] = u;
			}
			break;
		}
		case RsGenericSerializer::PRINT: break;
		case RsGenericSerializer::TO_JSON:
		{
			using namespace rapidjson;

			Document::AllocatorType& allocator = ctx.mJson.GetAllocator();
			Value arrKey; arrKey.SetString(
			            memberName.c_str(),
			            static_cast<SizeType>(memberName.length()), allocator );
			Value arr(kArrayType);

			for (auto& kv : member)
			{
				// Use same allocator to avoid deep copy
				RsGenericSerializer::SerializeContext kCtx(
				            nullptr, 0, ctx.mFlags, &allocator );
				serial_process(j, kCtx, const_cast<T&>(kv.first), "key");

				RsGenericSerializer::SerializeContext vCtx(
				            nullptr, 0, ctx.mFlags, &allocator );
				serial_process(j, vCtx, const_cast<U&>(kv.second), "value");

				if(kCtx.mOk && vCtx.mOk)
				{
					Value el(kObjectType);
					el.AddMember("key", kCtx.mJson["key"], allocator);
					el.AddMember("value", vCtx.mJson["value"], allocator);

					arr.PushBack(el, allocator);
				}
			}

			ctx.mJson.AddMember(arrKey, arr, allocator);

			break;
		}
		case RsGenericSerializer::FROM_JSON:
		{
			using namespace rapidjson;

			bool ok = ctx.mOk || !!(ctx.mFlags & RsSerializationFlags::YIELDING);
			Document& jDoc(ctx.mJson);
			Document::AllocatorType& allocator = jDoc.GetAllocator();

			Value arrKey;
			arrKey.SetString( memberName.c_str(),
			                  static_cast<SizeType>(memberName.length()) );

			ok = ok && jDoc.IsObject();
			ok = ok && jDoc.HasMember(arrKey);

			if(ok && jDoc[arrKey].IsArray())
			{
				for (auto&& kvEl : jDoc[arrKey].GetArray())
				{
					ok = ok && kvEl.IsObject();
					ok = ok && kvEl.HasMember("key");
					ok = ok && kvEl.HasMember("value");
					if (!ok) break;

					RsGenericSerializer::SerializeContext kCtx(
					            nullptr, 0, ctx.mFlags, &allocator );
					if(ok)
						kCtx.mJson.AddMember("key", kvEl["key"], allocator);

					T key;
					ok = ok && (serial_process(j, kCtx, key, "key"), kCtx.mOk);

					RsGenericSerializer::SerializeContext vCtx(
					            nullptr, 0, ctx.mFlags, &allocator );
					if(ok)
						vCtx.mJson.AddMember("value", kvEl["value"], allocator);

					U value;
					ok = ok && ( serial_process(j, vCtx, value, "value"),
					             vCtx.mOk );

					ctx.mOk &= ok;
					if(ok) member.insert(std::pair<T,U>(key,value));
					else break;
				}
			}
			else ctx.mOk = false;
			break;
		}
		default: fatalUnknownSerialJob(j);
		}
	}


//============================================================================//
//                               std::pair                                    //
//============================================================================//

	/// std::pair<T,U>
	template<typename T, typename U>
	static void serial_process(
	        RsGenericSerializer::SerializeJob j,
	        RsGenericSerializer::SerializeContext& ctx,
	        std::pair<T,U>& member, const std::string& memberName )
	{
		switch(j)
		{
		case RsGenericSerializer::SIZE_ESTIMATE: // [[fallthrough]]
		case RsGenericSerializer::DESERIALIZE: // [[fallthrough]]
		case RsGenericSerializer::SERIALIZE: // [[fallthrough]]
		case RsGenericSerializer::PRINT:
			RS_SERIAL_PROCESS(member.first);
			RS_SERIAL_PROCESS(member.second);
			break;
		case RsGenericSerializer::TO_JSON:
		{
			RsJson& jDoc(ctx.mJson);
			RsJson::AllocatorType& allocator = jDoc.GetAllocator();

			// Reuse allocator to avoid deep copy later
			RsGenericSerializer::SerializeContext lCtx(
			            nullptr, 0, ctx.mFlags, &allocator );

			serial_process(j, lCtx, member.first, "first");
			serial_process(j, lCtx, member.second, "second");

			rapidjson::Value key;
			key.SetString( memberName.c_str(),
			               static_cast<rapidjson::SizeType>(memberName.length()),
			               allocator);

			/* Because the passed allocator is reused it doesn't go out of scope
			 * and there is no need of deep copy and we can take advantage of
			 * the much faster rapidjson move semantic */
			jDoc.AddMember(key, lCtx.mJson, allocator);

			ctx.mOk = ctx.mOk && lCtx.mOk;
			break;
		}
		case RsGenericSerializer::FROM_JSON:
		{
			RsJson& jDoc(ctx.mJson);
			const char* mName = memberName.c_str();
			bool hasMember = jDoc.HasMember(mName);
			bool yielding = !!(ctx.mFlags & RsSerializationFlags::YIELDING);

			if(!hasMember)
			{
				if(!yielding)
				{
					RsErr() << __PRETTY_FUNCTION__ << " \"" << memberName
					         << "\" not found in JSON" << std::endl;
					print_stacktrace();
				}
				ctx.mOk = false;
				break;
			}

			rapidjson::Value& v = jDoc[mName];

			RsGenericSerializer::SerializeContext lCtx(nullptr, 0, ctx.mFlags);
			lCtx.mJson.SetObject() = v; // Beware of move semantic!!

			serial_process(j, lCtx, member.first, "first");
			serial_process(j, lCtx, member.second, "second");
			ctx.mOk &= lCtx.mOk;

			break;
		}
		default: fatalUnknownSerialJob(j);
		}
	}

//============================================================================//
//                       Sequence containers                                  //
//============================================================================//

	/** std::list is supported */ template <typename... Args>
	struct is_sequence_container<std::list<Args...>>: std::true_type {};

	/** std::set is supported */ template <typename... Args>
	struct is_sequence_container<std::set<Args...>>: std::true_type {};

	/**  std::vector is supported */ template <typename... Args>
	struct is_sequence_container<std::vector<Args...>>: std::true_type {};


	/// STL compatible sequence containers std::list, std::set, std::vector...
	template<typename T>
	typename std::enable_if<is_sequence_container<T>::value>::type
	static serial_process(
	        RsGenericSerializer::SerializeJob j,
	        RsGenericSerializer::SerializeContext& ctx,
	        T& member, const std::string& memberName )
	{
		using el_t = typename T::value_type;

		switch(j)
		{
		case RsGenericSerializer::SIZE_ESTIMATE: // [[falltrough]]
		case RsGenericSerializer::SERIALIZE:
		{
			uint32_t aSize = member.size();
			RS_SERIAL_PROCESS(aSize);
			for(auto it = member.begin(); it != member.end(); ++it)
			{
				if(!ctx.mOk) break;
				RS_SERIAL_PROCESS(*it);
			}
			break;
		}
		case RsGenericSerializer::DESERIALIZE:
		{
			uint32_t elCount = 0;
			RS_SERIAL_PROCESS(elCount);
			if(!ctx.mOk) break;

			/* This check is not perfect but will catch most pathological cases.
			 * Avoid multiplying by sizeof(el_t) as it is not a good exitimation
			 * of the actual serialized size, depending on the elements
			 * structure and on the so it would raises many false positives. */
			if(elCount > ctx.mSize - ctx.mOffset)
			{
				ctx.mOk = false;
				RsErr() << __PRETTY_FUNCTION__ << " attempt to deserialize a "
				        << "sequence with apparently malformed elements count."
				        << " elCount: " << elCount
				        << " ctx.mSize: " << ctx.mSize
				        << " ctx.mOffset: " << ctx.mOffset << " "
				        << std::errc::argument_out_of_domain
				        << std::endl;
				print_stacktrace();
				break;
			}

			for(uint32_t i=0; ctx.mOk && i < elCount; ++i )
			{
				el_t elem;
				RS_SERIAL_PROCESS(elem);
				member.insert(member.end(), elem);
			}
			break;
		}
		case RsGenericSerializer::PRINT: break;
		case RsGenericSerializer::TO_JSON:
		{
			using namespace rapidjson;

			Document::AllocatorType& allocator = ctx.mJson.GetAllocator();

			Value arrKey; arrKey.SetString(memberName.c_str(), allocator);
			Value arr(kArrayType);

			for(auto& const_el : member)
			{
				auto el = const_cast<el_t&>(const_el);

				/* Use same allocator to avoid deep copy */
				RsGenericSerializer::SerializeContext elCtx(
				            nullptr, 0, ctx.mFlags, &allocator );
				serial_process(j, elCtx, el, memberName);

				elCtx.mOk = elCtx.mOk && elCtx.mJson.HasMember(arrKey);
				if(elCtx.mOk) arr.PushBack(elCtx.mJson[arrKey], allocator);
				else
				{
					ctx.mOk = false;
					break;
				}
			}

			ctx.mJson.AddMember(arrKey, arr, allocator);
			break;
		}
		case RsGenericSerializer::FROM_JSON:
		{
			using namespace rapidjson;

			bool ok = ctx.mOk || !!(ctx.mFlags & RsSerializationFlags::YIELDING);
			Document& jDoc(ctx.mJson);
			Document::AllocatorType& allocator = jDoc.GetAllocator();

			Value arrKey;
			arrKey.SetString( memberName.c_str(),
			                  static_cast<SizeType>(memberName.length()) );

			ok = ok && jDoc.IsObject() && jDoc.HasMember(arrKey)
			        && jDoc[arrKey].IsArray();
			if(!ok) { ctx.mOk = false; break; }

			for (auto&& arrEl : jDoc[arrKey].GetArray())
			{
				Value arrKeyT;
				arrKeyT.SetString(
				            memberName.c_str(),
				            static_cast<SizeType>(memberName.length()) );

				RsGenericSerializer::SerializeContext elCtx(
				            nullptr, 0, ctx.mFlags, &allocator );
				elCtx.mJson.AddMember(arrKeyT, arrEl, allocator);

				el_t el;
				serial_process(j, elCtx, el, memberName);
				ok = ok && elCtx.mOk;
				ctx.mOk &= ok;
				if(ok) member.insert(member.end(), el);
				else break;
			}

			break;
		}
		default: fatalUnknownSerialJob(j);
		}
	}

//============================================================================//
//                               Strings                                      //
//============================================================================//

	/// Strings
	template<typename T>
	typename std::enable_if<is_string<T>::value>::type
	static serial_process(
	        RsGenericSerializer::SerializeJob j,
	        RsGenericSerializer::SerializeContext& ctx,
	        T& memberC,
	        const std::string& memberName )
	{
		// Avoid problems with const sneaking into template paramether
		using m_t = std::remove_const_t<T>;
		m_t& member = const_cast<m_t&>(memberC);

		switch(j)
		{
		case RsGenericSerializer::SIZE_ESTIMATE:
		{
			uint32_t aSize = static_cast<uint32_t>(member.size());
			RS_SERIAL_PROCESS(aSize);
			ctx.mOffset += aSize;
			break;
		}
		case RsGenericSerializer::SERIALIZE:
		{
			uint32_t len = static_cast<uint32_t>(member.length());
			RS_SERIAL_PROCESS(len);
			if(len > ctx.mSize - ctx.mOffset)
			{
				RsErr() << __PRETTY_FUNCTION__ << std::errc::no_buffer_space
				        << std::endl;
				ctx.mOk = false;
			}
			memcpy(ctx.mData + ctx.mOffset, member.c_str(), len);
			ctx.mOffset += len;
			break;
		}
		case RsGenericSerializer::DESERIALIZE:
		{
			uint32_t len;
			RS_SERIAL_PROCESS(len);
			if(!ctx.mOk) break;
			if(len > ctx.mSize - ctx.mOffset)
			{
				ctx.mOk = false;
				RsErr() << __PRETTY_FUNCTION__ << " attempt to deserialize a "
				        << "string with apparently malformed elements count."
				        << " len: " << len
				        << " ctx.mSize: " << ctx.mSize
				        << " ctx.mOffset: " << ctx.mOffset << " "
				        << std::errc::argument_out_of_domain
				        << std::endl;
				print_stacktrace();
				break;
			}
			member.resize(len);
			memcpy(&member[0], ctx.mData + ctx.mOffset, len);
			ctx.mOffset += len;
			break;
		}
		case RsGenericSerializer::PRINT: break;
		case RsGenericSerializer::TO_JSON:
			ctx.mOk = ctx.mOk && to_JSON(memberName, member, ctx.mJson);
			break;
		case RsGenericSerializer::FROM_JSON:
		{
			bool ok = ctx.mOk || !!(
			            ctx.mFlags & RsSerializationFlags::YIELDING );
			ctx.mOk = ok && from_JSON(memberName, member, ctx.mJson) && ctx.mOk;
			break;
		}
		default: fatalUnknownSerialJob(j);
		}
	}

	/// t_RsFlags32<> types
	template<int N>
	static void serial_process( RsGenericSerializer::SerializeJob j,
	                            RsGenericSerializer::SerializeContext& ctx,
	                            t_RsFlags32<N>& v,
	                            const std::string& memberName )
	{ serial_process(j, ctx, v._bits, memberName); }

	/**
	 * @brief serial process enum types
	 * On declaration of your member of enum type you must specify the
	 * underlying type otherwise the serialization format may differ in an
	 * uncompatible way depending on the compiler/platform.
	 */
	template<typename E>
	typename std::enable_if<std::is_enum<E>::value>::type
	static /*void*/ serial_process( RsGenericSerializer::SerializeJob j,
	                                RsGenericSerializer::SerializeContext& ctx,
	                                E& member,
	                                const std::string& memberName )
	{
#ifdef RSSERIAL_DEBUG
		std::cerr << __PRETTY_FUNCTION__  << " processing enum: "
		          << typeid(E).name() << " as "
		          << typeid(typename std::underlying_type<E>::type).name()
		          << std::endl;
#endif
		serial_process(
		      j, ctx,
		      reinterpret_cast<typename std::underlying_type<E>::type&>(member),
		      memberName );
	}

	/// RsSerializable and derivatives
	template<typename T>
	typename std::enable_if<std::is_base_of<RsSerializable,T>::value>::type
	static serial_process(
	        RsGenericSerializer::SerializeJob j,
	        RsGenericSerializer::SerializeContext& ctx,
	        T& memberC, const std::string& memberName )
	{
		using m_t = std::remove_const_t<T>;
		m_t& member = const_cast<m_t&>(memberC);

		switch(j)
		{
		case RsGenericSerializer::SIZE_ESTIMATE: // fallthrough
		case RsGenericSerializer::DESERIALIZE: // fallthrough
		case RsGenericSerializer::SERIALIZE: // fallthrough
		case RsGenericSerializer::PRINT:
			member.serial_process(j, ctx);
			break;
		case RsGenericSerializer::TO_JSON:
		{
			rapidjson::Document& jDoc(ctx.mJson);
			rapidjson::Document::AllocatorType& allocator = jDoc.GetAllocator();

			// Reuse allocator to avoid deep copy later
			RsGenericSerializer::SerializeContext lCtx(
			            nullptr, 0, ctx.mFlags, &allocator );

			member.serial_process(j, lCtx);

			rapidjson::Value key;
			key.SetString(
			            memberName.c_str(),
			            static_cast<rapidjson::SizeType>(memberName.length()),
			            allocator );

			/* Because the passed allocator is reused it doesn't go out of scope
			 * and there is no need of deep copy and we can take advantage of
			 * the much faster rapidjson move semantic */
			jDoc.AddMember(key, lCtx.mJson, allocator);

			ctx.mOk = ctx.mOk && lCtx.mOk;
			break;
		}
		case RsGenericSerializer::FROM_JSON:
		{
			RsJson& jDoc(ctx.mJson);
			const char* mName = memberName.c_str();
			bool hasMember = jDoc.HasMember(mName);
			bool yielding = !!(ctx.mFlags & RsSerializationFlags::YIELDING);

			if(!hasMember)
			{
				if(!yielding)
				{
					std::cerr << __PRETTY_FUNCTION__ << " \"" << memberName
					          << "\" not found in JSON:" << std::endl
					          << jDoc << std::endl << std::endl;
					print_stacktrace();
				}
				ctx.mOk = false;
				break;
			}

			rapidjson::Value& v = jDoc[mName];

			if(!v.IsObject())
			{
				std::cerr << __PRETTY_FUNCTION__ << " \"" << memberName
				          << "\" has wrong type in JSON, object expected, got:"
				          << std::endl << jDoc << std::endl << std::endl;
				print_stacktrace();
				ctx.mOk = false;
				break;
			}

			RsGenericSerializer::SerializeContext lCtx(nullptr, 0, ctx.mFlags);
			lCtx.mJson.SetObject() = v; // Beware of move semantic!!

			member.serial_process(j, lCtx);
			ctx.mOk &= lCtx.mOk;

			break;
		}
		default: fatalUnknownSerialJob(j);
		}
	}

	/// RsTlvItem derivatives only
	template<typename T>
	typename std::enable_if<
	    std::is_base_of<RsTlvItem,T>::value &&
	    !std::is_same<RsTlvItem,T>::value >::type
	static serial_process(
	        RsGenericSerializer::SerializeJob j,
	        RsGenericSerializer::SerializeContext& ctx,
	        T& member, const std::string& memberName )
	{ serial_process(j, ctx, static_cast<RsTlvItem&>(member), memberName); }

	/** std::error_condition
	 * supports only TO_JSON ErrConditionWrapper::serial_process will explode
	 * at runtime if a different SerializeJob is passed down */
	template<typename T>
	typename std::enable_if< std::is_base_of<std::error_condition,T>::value >::type
	static /*void*/ serial_process(
	        RsGenericSerializer::SerializeJob j,
	        RsGenericSerializer::SerializeContext& ctx,
	        T& cond, const std::string& member_name )
	{
		ErrConditionWrapper ew(cond);
		serial_process(j, ctx, ew, member_name);
	}

	RS_DEPRECATED_FOR(RawMemoryWrapper)
	typedef RawMemoryWrapper TlvMemBlock_proxy;

protected:

//============================================================================//
// Generic types declarations                                                 //
//============================================================================//

	template<typename T> static bool serialize(
	        uint8_t data[], uint32_t size, uint32_t &offset, const T& member );

	template<typename T> static bool deserialize(
	        const uint8_t data[], uint32_t size, uint32_t &offset, T& member);

	template<typename T> static uint32_t serial_size(const T& member);

	template<typename T> static void print_data(
	        const std::string& name, const T& member);

	template<typename T> static bool to_JSON( const std::string& membername,
	                                          const T& member, RsJson& jDoc );

	template<typename T> static bool from_JSON( const std::string& memberName,
	                                            T& member, RsJson& jDoc );

//============================================================================//
// Generic types + type_id declarations                                       //
//============================================================================//

	template<typename T> RS_DEPRECATED
	static bool serialize(
	        uint8_t data[], uint32_t size, uint32_t &offset, uint16_t type_id,
	        const T& member );

	template<typename T> RS_DEPRECATED
	static bool deserialize(
	        const uint8_t data[], uint32_t size, uint32_t &offset,
	        uint16_t type_id, T& member );

	template<typename T> RS_DEPRECATED
	static uint32_t serial_size(uint16_t type_id,const T& member);

	template<typename T> RS_DEPRECATED
	static void print_data(
	        const std::string& n, uint16_t type_id,const T& member );

	template<typename T> RS_DEPRECATED
	static bool to_JSON(
	        const std::string& membername, uint16_t type_id,
	        const T& member, RsJson& jVal );

	template<typename T> RS_DEPRECATED
	static bool from_JSON(
	        const std::string& memberName, uint16_t type_id,
	        T& member, RsJson& jDoc );

//============================================================================//
// t_RsGenericId<...> declarations                                            //
//============================================================================//

	template<uint32_t ID_SIZE_IN_BYTES, bool UPPER_CASE, RsGenericIdType UNIQUE_IDENTIFIER>
	static bool serialize(
	        uint8_t data[], uint32_t size, uint32_t &offset,
	        const t_RsGenericIdType<ID_SIZE_IN_BYTES, UPPER_CASE, UNIQUE_IDENTIFIER>& member );

	template<uint32_t ID_SIZE_IN_BYTES, bool UPPER_CASE, RsGenericIdType UNIQUE_IDENTIFIER>
	static bool deserialize(
	        const uint8_t data[], uint32_t size, uint32_t &offset,
	        t_RsGenericIdType<ID_SIZE_IN_BYTES, UPPER_CASE, UNIQUE_IDENTIFIER>& member );

	template<uint32_t ID_SIZE_IN_BYTES, bool UPPER_CASE, RsGenericIdType UNIQUE_IDENTIFIER>
	static uint32_t serial_size(
	        const t_RsGenericIdType<
	        ID_SIZE_IN_BYTES, UPPER_CASE, UNIQUE_IDENTIFIER>& member );

	template<uint32_t ID_SIZE_IN_BYTES,bool UPPER_CASE, RsGenericIdType UNIQUE_IDENTIFIER>
	static void print_data(
	        const std::string& name,
	        const t_RsGenericIdType<ID_SIZE_IN_BYTES,UPPER_CASE,UNIQUE_IDENTIFIER>& member );

	template<uint32_t ID_SIZE_IN_BYTES, bool UPPER_CASE, RsGenericIdType UNIQUE_IDENTIFIER>
	static bool to_JSON(
	        const std::string& membername,
	        const t_RsGenericIdType<ID_SIZE_IN_BYTES, UPPER_CASE, UNIQUE_IDENTIFIER>& member,
	        RsJson& jVal );

	template<uint32_t ID_SIZE_IN_BYTES, bool UPPER_CASE, RsGenericIdType UNIQUE_IDENTIFIER>
	static bool from_JSON(
	        const std::string& memberName,
	        t_RsGenericIdType<ID_SIZE_IN_BYTES, UPPER_CASE, UNIQUE_IDENTIFIER>& member,
	        RsJson& jDoc );

//============================================================================//
// t_RsTlvList<...> declarations                                              //
//============================================================================//

	template<class TLV_CLASS,uint32_t TLV_TYPE>
	static bool serialize(
	        uint8_t data[], uint32_t size, uint32_t &offset,
	        const t_RsTlvList<TLV_CLASS,TLV_TYPE>& member );

	template<class TLV_CLASS,uint32_t TLV_TYPE>
	static bool deserialize(
	        const uint8_t data[], uint32_t size, uint32_t &offset,
	        t_RsTlvList<TLV_CLASS,TLV_TYPE>& member );

	template<class TLV_CLASS,uint32_t TLV_TYPE>
	static uint32_t serial_size(const t_RsTlvList<TLV_CLASS,TLV_TYPE>& member);

	template<class TLV_CLASS,uint32_t TLV_TYPE>
	static void print_data(
	        const std::string& name,
	        const t_RsTlvList<TLV_CLASS,TLV_TYPE>& member);

	template<class TLV_CLASS,uint32_t TLV_TYPE>
	static bool to_JSON( const std::string& membername,
	                     const t_RsTlvList<TLV_CLASS,TLV_TYPE>& member,
	                     RsJson& jVal );

	template<class TLV_CLASS,uint32_t TLV_TYPE>
	static bool from_JSON( const std::string& memberName,
	                       t_RsTlvList<TLV_CLASS,TLV_TYPE>& member,
	                       RsJson& jDoc );

//============================================================================//
//                          Integral types VLQ                                //
//============================================================================//

	/**
	 * Size calculation of unsigned integers as Variable Lenght Quantity
	 * @see RsSerializationFlags::INTEGER_VLQ
	 * @see https://en.wikipedia.org/wiki/Variable-length_quantity
	 * @see https://golb.hplar.ch/2019/06/variable-length-int-java.html
	 * @see https://techoverflow.net/2013/01/25/efficiently-encoding-variable-length-integers-in-cc/
	 */
	template<typename T> static
	std::enable_if_t<std::is_unsigned<std::decay_t<T>>::value, uint32_t>
	VLQ_size(T member)
	{
		std::decay_t<T> memberBackup = member;
		uint32_t ret = 1;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wbool-compare"
		while(member > 127) { ++ret; member >>= 7; }
#pragma GCC diagnostic pop
		RsDbg() << __PRETTY_FUNCTION__ << " memberBackup: " << memberBackup
		        << " return: " << ret << std::endl;
		return ret;
	}

	/**
	 * Serialization of unsigned integers as Variable Lenght Quantity
	 * @see RsSerializationFlags::INTEGER_VLQ
	 * @see https://en.wikipedia.org/wiki/Variable-length_quantity
	 * @see https://golb.hplar.ch/2019/06/variable-length-int-java.html
	 * @see https://techoverflow.net/2013/01/25/efficiently-encoding-variable-length-integers-in-cc/
	 */
	template<typename T> static
	std::enable_if_t<std::is_unsigned<std::decay_t<T>>::value, bool>
	VLQ_serialize(
	        uint8_t data[], uint32_t size, uint32_t &offset, T member )
	{
		std::decay_t<T> backupMember = member;
		uint32_t offsetBackup = offset;

		bool ok = true;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wbool-compare"
		/* Check with < and not with <= here as we write last byte after
		 * the loop. Order of && operands very important here! */
		while(member > 127 && (ok = offset < size))
		{
			// | 128: Set the next byte flag
			data[offset++] = (static_cast<uint8_t>(member & 127)) | 128;
			// Remove the seven bits we just wrote
			member >>= 7;
		}
#pragma GCC diagnostic pop

		if(!(ok = ok && offset <= size))
		{
			RsErr() << __PRETTY_FUNCTION__ << " Cannot serialise "
			        << typeid(T).name()
			        << " member " << member
			        << " size: " << size
			        << " offset: " << offset
			        << std::error_condition(std::errc::no_buffer_space)
			        << std::endl;
			print_stacktrace();
			return false;
		}

		data[offset++] = static_cast<uint8_t>(member & 127);

		RsDbg() << __PRETTY_FUNCTION__ << " backupMember: " << backupMember
		        << " offsetBackup: " << offsetBackup << " offeset: " << offset
		        << " serialized as: ";
		for(; offsetBackup < offset; ++offsetBackup)
			std::cerr << " " << std::bitset<8>(data[offsetBackup]);
		std::cerr << std::endl;

		return ok;
	}

	/**
	 * Deserialization for unsigned integers as Variable Lenght Quantity
	 * @see RsSerializationFlags::INTEGER_VLQ
	 * @see https://en.wikipedia.org/wiki/Variable-length_quantity
	 * @see https://golb.hplar.ch/2019/06/variable-length-int-java.html
	 * @see https://techoverflow.net/2013/01/25/efficiently-encoding-variable-length-integers-in-cc/
	 */
	template<typename T> static
	std::enable_if_t<std::is_unsigned<std::decay_t<T>>::value, bool>
	VLQ_deserialize(
	        const uint8_t data[], uint32_t size, uint32_t& offset, T& member )
	{
		uint32_t backupOffset = offset;

		member = 0;
		uint32_t offsetBackup = offset;

		/* In a reasonable VLQ coding representing an integer
		 * could take at maximum sizeof(integer) + 1 space, if it is
		 * not the case something fishy is happening. */
		for (size_t i = 0; offset < size && i <= sizeof(T); ++i)
		{
			member |= (data[offset] & 127) << (7 * i);
			// If the next-byte flag is not set, ++ is after on purpose
			if(!(data[offset++] & 128))
			{
				RsDbg() << __PRETTY_FUNCTION__
				        << " size: " << size
				        << " backupOffset " << backupOffset
				        << " offset: " << offset
				        << " member " << member << std::endl;
				return true;
			}
		}

		/* If return is not triggered inside the for loop, either the buffer
		 * ended before we encountered the end of the number, or the number
		 * is VLQ encoded improperly */
		RsErr() << __PRETTY_FUNCTION__ << std::errc::illegal_byte_sequence
		        << " offsetBackup: " << offsetBackup
		        << " offset: " << offset << " bytes: ";
		for(; offsetBackup < offset; ++offsetBackup)
			std::cerr << " " << std::bitset<8>(data[offsetBackup]);
		std::cerr << std::endl;

		print_stacktrace();
		return false;
	}

	/**
	 * Size calculation of signed integers as Variable Lenght Quantity
	 * @see RsSerializationFlags::INTEGER_VLQ
	 * @see https://en.wikipedia.org/wiki/Variable-length_quantity#Zigzag_encoding
	 * @see https://golb.hplar.ch/2019/06/variable-length-int-java.html
	 */
	template<typename T> static
	std::enable_if_t<std::is_signed<std::decay_t<T>>::value, uint32_t>
	VLQ_size(T member)
	{
		member = (member << 1) ^ (member >> (sizeof(T)-1)); // ZigZag encoding
		return VLQ_size(
		            static_cast<typename std::make_unsigned<T>::type>(member));
	}

	/**
	 * Serialization of signed integers as Variable Lenght Quantity
	 * @see RsSerializationFlags::INTEGER_VLQ
	 * @see https://en.wikipedia.org/wiki/Variable-length_quantity#Zigzag_encoding
	 * @see https://golb.hplar.ch/2019/06/variable-length-int-java.html
	 */
	template<typename T> static
	std::enable_if_t<std::is_signed<std::decay_t<T>>::value, bool>
	VLQ_serialize(
	        uint8_t data[], uint32_t size, uint32_t &offset, T member )
	{
		member = (member << 1) ^ (member >> (sizeof(T)-1)); // ZigZag encoding
		return VLQ_serialize(
		            data, size, offset,
		            static_cast<typename std::make_unsigned<T>::type>(member));
	}

	/**
	 * Deserialization for signed integers as Variable Lenght Quantity
	 * @see RsSerializationFlags::INTEGER_VLQ
	 * @see https://en.wikipedia.org/wiki/Variable-length_quantity#Zigzag_encoding
	 * @see https://golb.hplar.ch/2019/06/variable-length-int-java.html
	 */
	template<typename T> static
	std::enable_if_t<std::is_signed<std::decay_t<T>>::value, bool>
	VLQ_deserialize(
	        const uint8_t data[], uint32_t size, uint32_t& offset, T& member )
	{
		using DT = std::decay_t<T>;
		typename std::make_unsigned<DT>::type temp = 0;
		bool ok = VLQ_deserialize(data, size, offset, temp);
		// ZizZag decoding
		member = (static_cast<DT>(temp) >> 1) ^ -(static_cast<DT>(temp) & 1);
		return ok;
	}

//============================================================================//
//                        Error Condition Wrapper                             //
//============================================================================//

	struct ErrConditionWrapper : RsSerializable
	{
		ErrConditionWrapper(const std::error_condition& ec): mec(ec) {}

		/** supports only TO_JSON if a different SerializeJob is passed it will
		 * explode at runtime */
		void serial_process(
		        RsGenericSerializer::SerializeJob j,
		        RsGenericSerializer::SerializeContext& ctx ) override;

	private:
		const std::error_condition& mec;
	};

//============================================================================//
//                                Miscellanea                                 //
//============================================================================//

	[[noreturn]] static void fatalUnknownSerialJob(int j)
	{
		RsFatal() << " Unknown serial job: " << j << std::endl;
		print_stacktrace();
		exit(EINVAL);
	}

	RS_SET_CONTEXT_DEBUG_LEVEL(1)
};


//============================================================================//
//                            t_RsGenericId<...>                              //
//============================================================================//

template<uint32_t ID_SIZE_IN_BYTES, bool UPPER_CASE, RsGenericIdType UNIQUE_IDENTIFIER>
bool RsTypeSerializer::serialize (
        uint8_t data[], uint32_t size, uint32_t &offset,
        const t_RsGenericIdType<
        ID_SIZE_IN_BYTES, UPPER_CASE, UNIQUE_IDENTIFIER>& member )
{
	return (*const_cast<const t_RsGenericIdType<
	      ID_SIZE_IN_BYTES, UPPER_CASE, UNIQUE_IDENTIFIER> *>(&member)
	      ).serialise(data, size, offset);
}

template<uint32_t ID_SIZE_IN_BYTES, bool UPPER_CASE, RsGenericIdType UNIQUE_IDENTIFIER>
bool RsTypeSerializer::deserialize(
        const uint8_t data[], uint32_t size, uint32_t &offset,
        t_RsGenericIdType<ID_SIZE_IN_BYTES, UPPER_CASE, UNIQUE_IDENTIFIER>& member )
{ return member.deserialise(data, size, offset); }

template<uint32_t ID_SIZE_IN_BYTES, bool UPPER_CASE, RsGenericIdType UNIQUE_IDENTIFIER>
uint32_t RsTypeSerializer::serial_size(
        const t_RsGenericIdType<ID_SIZE_IN_BYTES, UPPER_CASE, UNIQUE_IDENTIFIER>& member )
{ return member.serial_size(); }

template<uint32_t ID_SIZE_IN_BYTES, bool UPPER_CASE, RsGenericIdType UNIQUE_IDENTIFIER>
void RsTypeSerializer::print_data(
        const std::string& /*name*/,
        const t_RsGenericIdType<ID_SIZE_IN_BYTES, UPPER_CASE, UNIQUE_IDENTIFIER>& member )
{
	std::cerr << "  [RsGenericId<" << std::hex
	          << static_cast<uint32_t>(UNIQUE_IDENTIFIER) << ">] : "
	          << member << std::endl;
}

template<uint32_t ID_SIZE_IN_BYTES, bool UPPER_CASE, RsGenericIdType UNIQUE_IDENTIFIER>
bool RsTypeSerializer::to_JSON(
        const std::string& memberName,
        const t_RsGenericIdType<ID_SIZE_IN_BYTES, UPPER_CASE, UNIQUE_IDENTIFIER>& member,
        RsJson& jDoc )
{
	rapidjson::Document::AllocatorType& allocator = jDoc.GetAllocator();

	rapidjson::Value key;
	key.SetString(memberName.c_str(), memberName.length(), allocator);

	const std::string vStr = member.toStdString();
	rapidjson::Value value;
	value.SetString(vStr.c_str(), vStr.length(), allocator);

	jDoc.AddMember(key, value, allocator);

	return true;
}

template<uint32_t ID_SIZE_IN_BYTES, bool UPPER_CASE, RsGenericIdType UNIQUE_IDENTIFIER>
bool RsTypeSerializer::from_JSON(
        const std::string& membername,
        t_RsGenericIdType<ID_SIZE_IN_BYTES, UPPER_CASE, UNIQUE_IDENTIFIER>& member,
        RsJson& jVal )
{
	const char* mName = membername.c_str();
	bool ret = jVal.HasMember(mName);
	if(ret)
	{
		rapidjson::Value& v = jVal[mName];
		ret = ret && v.IsString();
		if(ret) member =
		        t_RsGenericIdType<ID_SIZE_IN_BYTES,UPPER_CASE,UNIQUE_IDENTIFIER>(
		            std::string(v.GetString()) );
	}
	return ret;
}

//============================================================================//
//                             t_RsTlvList<...>                               //
//============================================================================//

template<class TLV_CLASS,uint32_t TLV_TYPE>
bool RsTypeSerializer::serialize(
        uint8_t data[], uint32_t size, uint32_t &offset,
        const t_RsTlvList<TLV_CLASS,TLV_TYPE>& member )
{
	return (*const_cast<const t_RsTlvList<TLV_CLASS,TLV_TYPE> *>(&member)).SetTlv(data,size,&offset);
}

template<class TLV_CLASS,uint32_t TLV_TYPE>
bool RsTypeSerializer::deserialize(
        const uint8_t data[], uint32_t size, uint32_t &offset,
        t_RsTlvList<TLV_CLASS,TLV_TYPE>& member )
{
	return member.GetTlv(const_cast<uint8_t*>(data),size,&offset);
}

template<class TLV_CLASS,uint32_t TLV_TYPE>
uint32_t RsTypeSerializer::serial_size(
        const t_RsTlvList<TLV_CLASS,TLV_TYPE>& member)
{ return member.TlvSize(); }

template<class TLV_CLASS,uint32_t TLV_TYPE>
void RsTypeSerializer::print_data(
        const std::string& /*name*/,
        const t_RsTlvList<TLV_CLASS,TLV_TYPE>& member)
{
	std::cerr << "  [t_RsTlvString<" << std::hex << TLV_TYPE << ">] : size="
	          << member.mList.size() << std::endl;
}

template<class TLV_CLASS,uint32_t TLV_TYPE> /* static */
bool RsTypeSerializer::to_JSON( const std::string& memberName,
                                const t_RsTlvList<TLV_CLASS,TLV_TYPE>& member,
                                RsJson& jDoc )
{
	rapidjson::Document::AllocatorType& allocator = jDoc.GetAllocator();

	rapidjson::Value key;
	key.SetString(memberName.c_str(), memberName.length(), allocator);

	rapidjson::Value value;
	const char* tName = typeid(member).name();
	value.SetString(tName, allocator);

	jDoc.AddMember(key, value, allocator);

	std::cerr << __PRETTY_FUNCTION__ << " JSON serialization for type "
	          << typeid(member).name() << " " << memberName
	          << " not available." << std::endl;
	print_stacktrace();
	return true;
}

template<class TLV_CLASS,uint32_t TLV_TYPE>
bool RsTypeSerializer::from_JSON( const std::string& memberName,
                                  t_RsTlvList<TLV_CLASS,TLV_TYPE>& member,
                                  RsJson& /*jVal*/ )
{
	std::cerr << __PRETTY_FUNCTION__ << " JSON deserialization for type "
	          << typeid(member).name() << " " << memberName
	          << " not available." << std::endl;
	print_stacktrace();
	return true;
}

//============================================================================//
// Not implemented types macros                                               //
//============================================================================//

/**
 * @def RS_TYPE_SERIALIZER_TO_JSON_NOT_IMPLEMENTED_DEF(T)
 * @def RS_TYPE_SERIALIZER_FROM_JSON_NOT_IMPLEMENTED_DEF(T)
 * Helper macros for types that has not yet implemented to/from JSON
 * should be deleted from the code as soon as they are not needed anymore
 */
#define RS_TYPE_SERIALIZER_TO_JSON_NOT_IMPLEMENTED_DEF(T) \
template<> /*static*/ \
bool RsTypeSerializer::to_JSON(const std::string& memberName, T const& member, \
	                            RsJson& jDoc ) \
{ \
	rapidjson::Document::AllocatorType& allocator = jDoc.GetAllocator(); \
 \
	rapidjson::Value key; \
	key.SetString(memberName.c_str(), memberName.length(), allocator); \
 \
	rapidjson::Value value; \
	const char* tName = typeid(member).name(); \
	value.SetString(tName, allocator); \
 \
	jDoc.AddMember(key, value, allocator); \
 \
	std::cerr << __FILE__ << __LINE__ << __PRETTY_FUNCTION__ \
	          << " JSON serialization for type " \
	          << typeid(member).name() << " " << memberName \
	          << " not available." << std::endl; \
	print_stacktrace(); \
	return true; \
}

#define RS_TYPE_SERIALIZER_FROM_JSON_NOT_IMPLEMENTED_DEF(T) \
template<> /*static*/ \
bool RsTypeSerializer::from_JSON( const std::string& memberName, \
	                              T& member, RsJson& /*jDoc*/ ) \
{ \
	std::cerr << __FILE__ << __LINE__ << __PRETTY_FUNCTION__ \
	          << " JSON deserialization for type " \
	          << typeid(member).name() << " " << memberName \
	          << " not available." << std::endl; \
	print_stacktrace(); \
	return true; \
}
