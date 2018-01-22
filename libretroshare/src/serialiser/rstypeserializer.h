/*
 * libretroshare/src/serialiser: rstypeserializer.h
 *
 * RetroShare Serialiser.
 *
 * Copyright (C) 2017  Cyril Soler
 * Copyright (C) 2018  Gioacchino Mazzurco <gio@eigenlab.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 *
 * Please report all bugs and problems to "csoler@users.sourceforge.net".
 *
 */
#pragma once

#include "serialiser/rsserial.h"
#include "serialiser/rstlvbase.h"
#include "serialiser/rstlvlist.h"

#include "retroshare/rsflags.h"
#include "retroshare/rsids.h"

#include "serialiser/rsserializer.h"
#include "serialiser/rsserializable.h"

#include <rapidjson/document.h>
#include <typeinfo> // for typeid


/** INTERNAL ONLY helper to avoid copy paste code for std::{vector,list,set}<T>
 * Can't use a template function because T is needed for const_cast */
#define RsTypeSerializer_PRIVATE_TO_JSON_ARRAY() do \
{ \
	using namespace rapidjson; \
\
	Document::AllocatorType& allocator = ctx.mJson.GetAllocator(); \
\
	Value arrKey; arrKey.SetString(memberName.c_str(), allocator); \
\
	Value arr(kArrayType); \
\
	for (auto& el : v) \
    { \
	    /* Use same allocator to avoid deep copy */\
	    RsGenericSerializer::SerializeContext elCtx(\
	                nullptr, 0, RsGenericSerializer::FORMAT_BINARY,\
	                RsGenericSerializer::SERIALIZATION_FLAG_NONE,\
	                &allocator );\
\
	    /* If el is const the default serial_process template is matched */ \
	    /* also when specialization is necessary so the compilation break */ \
	    serial_process(j, elCtx, const_cast<T&>(el), memberName); \
\
	    elCtx.mOk = elCtx.mOk && elCtx.mJson.HasMember(arrKey);\
	    if(elCtx.mOk) arr.PushBack(elCtx.mJson[arrKey], allocator);\
	    else\
        {\
	        ctx.mOk = false;\
	        break;\
	    }\
	}\
\
	ctx.mJson.AddMember(arrKey, arr, allocator);\
} while (false)

/** INTERNAL ONLY helper to avoid copy paste code for std::{vector,list,set}<T>
 * Can't use a template function because std::{vector,list,set}<T> has different
 * name for insert/push_back function
 */
#define RsTypeSerializer_PRIVATE_FROM_JSON_ARRAY(INSERT_FUN) do\
{\
	using namespace rapidjson;\
\
	bool& ok(ctx.mOk);\
	Document& jDoc(ctx.mJson);\
	Document::AllocatorType& allocator = jDoc.GetAllocator();\
\
	Value arrKey;\
	arrKey.SetString(memberName.c_str(), memberName.length());\
\
	ok = ok && jDoc.IsObject();\
	ok = ok && jDoc.HasMember(arrKey);\
\
	if(ok && jDoc[arrKey].IsArray())\
    {\
	    for (auto&& arrEl : jDoc[arrKey].GetArray())\
        {\
	        RsGenericSerializer::SerializeContext elCtx(\
	                    nullptr, 0, RsGenericSerializer::FORMAT_BINARY,\
	                    RsGenericSerializer::SERIALIZATION_FLAG_NONE,\
	                    &allocator );\
	        elCtx.mJson.AddMember(arrKey, arrEl, allocator);\
\
	        T el;\
	        serial_process(j, elCtx, el, memberName); \
	        ok = ok && elCtx.mOk;\
\
	        if(ok) v.INSERT_FUN(el);\
	        else break;\
	    }\
	}\
} while(false)

std::ostream &operator<<(std::ostream &out, const RsJson &jDoc);

struct RsTypeSerializer
{
	/** This type should be used to pass a parameter to drive the serialisation
	 * if needed */
	struct TlvMemBlock_proxy : std::pair<void*&,uint32_t&>
	{
		TlvMemBlock_proxy(void*& p, uint32_t& s) :
		    std::pair<void*&,uint32_t&>(p,s) {}
		TlvMemBlock_proxy(uint8_t*& p,uint32_t& s) :
		    std::pair<void*&,uint32_t&>(*(void**)&p,s) {}
	};

	/// Generic types
	template<typename T>
	static void serial_process( RsGenericSerializer::SerializeJob j,
	                            RsGenericSerializer::SerializeContext& ctx,
	                            T& member, const std::string& member_name )
	{
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
			ctx.mOk = ctx.mOk && from_JSON(member_name, member, ctx.mJson);
			break;
		default:
			ctx.mOk = false;
			throw std::runtime_error("Unknown serial job");
		}
	}

	/// Generic types + type_id
	template<typename T>
	static void serial_process( RsGenericSerializer::SerializeJob j,
	                            RsGenericSerializer::SerializeContext& ctx,
	                            uint16_t type_id, T& member,
	                            const std::string& member_name )
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
		case RsGenericSerializer::PRINT:
			print_data(member_name, member);
			break;
		case RsGenericSerializer::TO_JSON:
			ctx.mOk = ctx.mOk &&
			        to_JSON(member_name, type_id, member, ctx.mJson);
			break;
		case RsGenericSerializer::FROM_JSON:
			ctx.mOk = ctx.mOk &&
			        from_JSON(member_name, type_id, member, ctx.mJson);
			break;
		default:
			ctx.mOk = false;
			throw std::runtime_error("Unknown serial job");
		}
	}

	/// std::map<T,U>
	template<typename T,typename U>
	static void serial_process( RsGenericSerializer::SerializeJob j,
	                            RsGenericSerializer::SerializeContext& ctx,
	                            std::map<T,U>& v,
	                            const std::string& memberName )
	{
		switch(j)
		{
		case RsGenericSerializer::SIZE_ESTIMATE:
		{
			ctx.mOffset += 4;
			for(typename std::map<T,U>::iterator it(v.begin());it!=v.end();++it)
			{
				serial_process( j, ctx, const_cast<T&>(it->first),
				                "map::*it->first" );
				serial_process( j,ctx,const_cast<U&>(it->second),
				                "map::*it->second" );
			}
			break;
		}
		case RsGenericSerializer::DESERIALIZE:
		{
			uint32_t n=0;
			serial_process(j,ctx,n,"temporary size");

			for(uint32_t i=0; i<n; ++i)
			{
				T t; U u;
				serial_process(j, ctx, t, "map::*it->first");
				serial_process(j, ctx, u, "map::*it->second");
				v[t] = u;
			}
			break;
		}
		case RsGenericSerializer::SERIALIZE:
		{
			uint32_t n=v.size();
			serial_process(j,ctx,n,"temporary size");

			for(typename std::map<T,U>::iterator it(v.begin());it!=v.end();++it)
			{
				serial_process( j, ctx, const_cast<T&>(it->first),
				                "map::*it->first" );
				serial_process( j, ctx, const_cast<U&>(it->second),
				                "map::*it->second" );
			}
			break;
		}
		case RsGenericSerializer::PRINT:
		{
			if(v.empty())
				std::cerr << "  Empty map \"" << memberName << "\""
				          << std::endl;
			else
				std::cerr << "  std::map of " << v.size() << " elements: \""
				          << memberName << "\"" << std::endl;

			for(typename std::map<T,U>::iterator it(v.begin());it!=v.end();++it)
			{
				std::cerr << "  ";

				serial_process( j, ctx,const_cast<T&>(it->first),
				                "map::*it->first" );
				serial_process( j, ctx, const_cast<U&>(it->second),
				                "map::*it->second" );
			}
			break;
		}
		case RsGenericSerializer::TO_JSON:
		{
			using namespace rapidjson;

			Document::AllocatorType& allocator = ctx.mJson.GetAllocator();
			Value arrKey; arrKey.SetString(memberName.c_str(),
			                               memberName.length(), allocator);
			Value arr(kArrayType);

			for (auto& kv : v)
			{
				// Use same allocator to avoid deep copy
				RsGenericSerializer::SerializeContext kCtx(
				            nullptr, 0, RsGenericSerializer::FORMAT_BINARY,
				            RsGenericSerializer::SERIALIZATION_FLAG_NONE,
				            &allocator );
				serial_process<T>(j, kCtx, const_cast<T&>(kv.first), "key");

				RsGenericSerializer::SerializeContext vCtx(
				            nullptr, 0, RsGenericSerializer::FORMAT_BINARY,
				            RsGenericSerializer::SERIALIZATION_FLAG_NONE,
				            &allocator );
				serial_process<U>(j, vCtx, const_cast<U&>(kv.second), "value");

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

			bool& ok(ctx.mOk);
			Document& jDoc(ctx.mJson);
			Document::AllocatorType& allocator = jDoc.GetAllocator();

			Value arrKey;
			arrKey.SetString(memberName.c_str(), memberName.length());

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
					            nullptr, 0, RsGenericSerializer::FORMAT_BINARY,
					            RsGenericSerializer::SERIALIZATION_FLAG_NONE,
					            &allocator );
					ok && (kCtx.mJson.
					       AddMember("key", kvEl["key"], allocator), true);

					T key;
					ok = ok && (serial_process(j, kCtx, key, "key"), kCtx.mOk);

					RsGenericSerializer::SerializeContext vCtx(
					            nullptr, 0, RsGenericSerializer::FORMAT_BINARY,
					            RsGenericSerializer::SERIALIZATION_FLAG_NONE,
					            &allocator );
					ok && (vCtx.mJson.
					       AddMember("value", kvEl["value"], allocator), true);

					U value;
					ok = ok && ( serial_process(j, vCtx, value, "value"),
					             vCtx.mOk );

					if(ok) v.insert(std::pair<T,U>(key,value));
					else break;
				}
			}

			break;
		}
		default: break;
		}
	}

	/// std::vector<T>
	template<typename T>
	static void serial_process( RsGenericSerializer::SerializeJob j,
	                            RsGenericSerializer::SerializeContext& ctx,
	                            std::vector<T>& v,
	                            const std::string& memberName )
	{
		switch(j)
		{
		case RsGenericSerializer::SIZE_ESTIMATE:
		{
			ctx.mOffset += 4;
			for(uint32_t i=0;i<v.size();++i)
				serial_process(j,ctx,v[i],memberName);
			break;
		}
		case RsGenericSerializer::DESERIALIZE:
		{
			uint32_t n=0;
			serial_process(j,ctx,n,"temporary size");
			v.resize(n);
			for(uint32_t i=0;i<v.size();++i)
				serial_process(j,ctx,v[i],memberName);
			break;
		}
		case RsGenericSerializer::SERIALIZE:
		{
			uint32_t n=v.size();
			serial_process(j,ctx,n,"temporary size");
			for(uint32_t i=0; i<v.size(); ++i)
				serial_process(j,ctx,v[i],memberName);
			break;
		}
		case RsGenericSerializer::PRINT:
		{
			if(v.empty())
				std::cerr << "  Empty array"<< std::endl;
			else
				std::cerr << "  Array of " << v.size() << " elements:"
				          << std::endl;
			for(uint32_t i=0;i<v.size();++i)
			{
				std::cerr << "  " ;
				serial_process(j,ctx,v[i],memberName);
			}
			break;
		}
		case RsGenericSerializer::TO_JSON:
			RsTypeSerializer_PRIVATE_TO_JSON_ARRAY();
			break;
		case RsGenericSerializer::FROM_JSON:
			RsTypeSerializer_PRIVATE_FROM_JSON_ARRAY(push_back);
			break;
		default: break;
		}
	}

	/// std::set<T>
	template<typename T>
	static void serial_process( RsGenericSerializer::SerializeJob j,
	                            RsGenericSerializer::SerializeContext& ctx,
	                            std::set<T>& v, const std::string& memberName )
	{
		switch(j)
		{
		case RsGenericSerializer::SIZE_ESTIMATE:
		{
			ctx.mOffset += 4;
			for(typename std::set<T>::iterator it(v.begin());it!=v.end();++it)
				// the const cast here is a hack to avoid serial_process to
				// instantiate serialise(const T&)
				serial_process(j,ctx,const_cast<T&>(*it) ,memberName);
			break;
		}
		case RsGenericSerializer::DESERIALIZE:
		{
			uint32_t n=0;
			serial_process(j,ctx,n,"temporary size");
			for(uint32_t i=0; i<n; ++i)
			{
				T tmp;
				serial_process<T>(j,ctx,tmp,memberName);
				v.insert(tmp);
			}
			break;
		}
		case RsGenericSerializer::SERIALIZE:
		{
			uint32_t n=v.size();
			serial_process(j,ctx,n,"temporary size");
			for(typename std::set<T>::iterator it(v.begin());it!=v.end();++it)
				// the const cast here is a hack to avoid serial_process to
				// instantiate serialise(const T&)
				serial_process(j,ctx,const_cast<T&>(*it) ,memberName);
			break;
		}
		case RsGenericSerializer::PRINT:
		{
			if(v.empty()) std::cerr << "  Empty set"<< std::endl;
			else std::cerr << "  Set of " << v.size() << " elements:"
			               << std::endl;
			break;
		}
		case RsGenericSerializer::TO_JSON:
			RsTypeSerializer_PRIVATE_TO_JSON_ARRAY();
			break;
		case RsGenericSerializer::FROM_JSON:
			RsTypeSerializer_PRIVATE_FROM_JSON_ARRAY(insert);
			break;
		default: break;
		}
	}

	/// std::list<T>
	template<typename T>
	static void serial_process( RsGenericSerializer::SerializeJob j,
	                            RsGenericSerializer::SerializeContext& ctx,
	                            std::list<T>& v,
	                            const std::string& memberName )
	{
		switch(j)
		{
		case RsGenericSerializer::SIZE_ESTIMATE:
		{
			ctx.mOffset += 4;
			for(typename std::list<T>::iterator it(v.begin());it!=v.end();++it)
				serial_process(j,ctx,*it ,memberName);
			break;
		}
		case RsGenericSerializer::DESERIALIZE:
		{
			uint32_t n=0;
			serial_process(j,ctx,n,"temporary size");
			for(uint32_t i=0;i<n;++i)
			{
				T tmp;
				serial_process<T>(j,ctx,tmp,memberName);
				v.push_back(tmp);
			}
			break;
		}
		case RsGenericSerializer::SERIALIZE:
		{
			uint32_t n=v.size();
			serial_process(j,ctx,n,"temporary size");
			for(typename std::list<T>::iterator it(v.begin());it!=v.end();++it)
				serial_process(j,ctx,*it ,memberName);
			break;
		}
		case RsGenericSerializer::PRINT:
		{
			if(v.empty()) std::cerr << "  Empty list"<< std::endl;
			else std::cerr << "  List of " << v.size() << " elements:"
			               << std::endl;
			break;
		}
		case RsGenericSerializer::TO_JSON:
			RsTypeSerializer_PRIVATE_TO_JSON_ARRAY();
			break;
		case RsGenericSerializer::FROM_JSON:
			RsTypeSerializer_PRIVATE_FROM_JSON_ARRAY(push_back);
			break;
		default: break;
		}
	}

	/// t_RsFlags32<> types
	template<int N>
	static void serial_process( RsGenericSerializer::SerializeJob j,
	                            RsGenericSerializer::SerializeContext& ctx,
	                            t_RsFlags32<N>& v,
	                            const std::string& memberName )
	{
		switch(j)
		{
		case RsGenericSerializer::SIZE_ESTIMATE: ctx.mOffset += 4; break;
		case RsGenericSerializer::DESERIALIZE:
		{
			uint32_t n=0;
			deserialize<uint32_t>(ctx.mData,ctx.mSize,ctx.mOffset,n);
			v = t_RsFlags32<N>(n);
			break;
		}
		case RsGenericSerializer::SERIALIZE:
		{
			uint32_t n=v.toUInt32();
			serialize<uint32_t>(ctx.mData,ctx.mSize,ctx.mOffset,n);
			break;
		}
		case RsGenericSerializer::PRINT:
			std::cerr << "  Flags of type " << std::hex << N << " : "
			          << v.toUInt32() << std::endl;
			break;
		case RsGenericSerializer::TO_JSON:
			ctx.mOk = to_JSON(memberName, v.toUInt32(), ctx.mJson);
			break;
		case RsGenericSerializer::FROM_JSON:
		{
			uint32_t f;
			ctx.mOk = from_JSON(memberName, f, ctx.mJson);
			v = t_RsFlags32<N>(f);
			break;
		}
		default: break;
		}
	}


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

	template<typename T> static bool serialize(
	        uint8_t data[], uint32_t size, uint32_t &offset, uint16_t type_id,
	        const T& member );

	template<typename T> static bool deserialize(
	        const uint8_t data[], uint32_t size, uint32_t &offset,
	        uint16_t type_id, T& member );

	template<typename T> static uint32_t serial_size(
	        uint16_t type_id,const T& member );

	template<typename T> static void print_data( const std::string& n,
	        uint16_t type_id,const T& member );

	template<typename T> static bool to_JSON( const std::string& membername,
	                                          uint16_t type_id,
	                                          const T& member, RsJson& jVal );

	template<typename T> static bool from_JSON( const std::string& memberName,
	                                            uint16_t type_id,
	                                            T& member, RsJson& jDoc );

//============================================================================//
// t_RsGenericId<...> declarations                                            //
//============================================================================//

	template<uint32_t ID_SIZE_IN_BYTES,bool UPPER_CASE,uint32_t UNIQUE_IDENTIFIER>
	static bool serialize(
	        uint8_t data[], uint32_t size, uint32_t &offset,
	        const t_RsGenericIdType<ID_SIZE_IN_BYTES,UPPER_CASE,UNIQUE_IDENTIFIER>& member );

	template<uint32_t ID_SIZE_IN_BYTES,bool UPPER_CASE,uint32_t UNIQUE_IDENTIFIER>
	static bool deserialize(
	        const uint8_t data[], uint32_t size, uint32_t &offset,
	        t_RsGenericIdType<ID_SIZE_IN_BYTES,UPPER_CASE,UNIQUE_IDENTIFIER>& member );

	template<uint32_t ID_SIZE_IN_BYTES,bool UPPER_CASE,uint32_t UNIQUE_IDENTIFIER>
	static uint32_t serial_size(
	        const t_RsGenericIdType<
	        ID_SIZE_IN_BYTES,UPPER_CASE,UNIQUE_IDENTIFIER>& member );

	template<uint32_t ID_SIZE_IN_BYTES,bool UPPER_CASE,uint32_t UNIQUE_IDENTIFIER>
	static void print_data(
	        const std::string& name,
	        const t_RsGenericIdType<ID_SIZE_IN_BYTES,UPPER_CASE,UNIQUE_IDENTIFIER>& member );

	template<uint32_t ID_SIZE_IN_BYTES,bool UPPER_CASE,uint32_t UNIQUE_IDENTIFIER>
	static bool to_JSON(
	        const std::string& membername,
	        const t_RsGenericIdType<ID_SIZE_IN_BYTES,UPPER_CASE,UNIQUE_IDENTIFIER>& member,
	        RsJson& jVal );

	template<uint32_t ID_SIZE_IN_BYTES,bool UPPER_CASE,uint32_t UNIQUE_IDENTIFIER>
	static bool from_JSON(
	        const std::string& memberName,
	        t_RsGenericIdType<ID_SIZE_IN_BYTES,UPPER_CASE,UNIQUE_IDENTIFIER>& member,
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
};


//============================================================================//
//                            t_RsGenericId<...>                              //
//============================================================================//

template<uint32_t ID_SIZE_IN_BYTES,bool UPPER_CASE,uint32_t UNIQUE_IDENTIFIER>
bool RsTypeSerializer::serialize (
        uint8_t data[], uint32_t size, uint32_t &offset,
        const t_RsGenericIdType<
        ID_SIZE_IN_BYTES,UPPER_CASE,UNIQUE_IDENTIFIER>& member )
{
	return (*const_cast<const t_RsGenericIdType<
	      ID_SIZE_IN_BYTES,UPPER_CASE,UNIQUE_IDENTIFIER> *>(&member)
	      ).serialise(data,size,offset);
}

template<uint32_t ID_SIZE_IN_BYTES,bool UPPER_CASE,uint32_t UNIQUE_IDENTIFIER>
bool RsTypeSerializer::deserialize(
        const uint8_t data[], uint32_t size, uint32_t &offset,
        t_RsGenericIdType<ID_SIZE_IN_BYTES,UPPER_CASE,UNIQUE_IDENTIFIER>& member )
{ return member.deserialise(data,size,offset); }

template<uint32_t ID_SIZE_IN_BYTES,bool UPPER_CASE,uint32_t UNIQUE_IDENTIFIER>
uint32_t RsTypeSerializer::serial_size(
        const t_RsGenericIdType<ID_SIZE_IN_BYTES,UPPER_CASE,UNIQUE_IDENTIFIER>& member )
{ return member.serial_size(); }

template<uint32_t ID_SIZE_IN_BYTES,bool UPPER_CASE,uint32_t UNIQUE_IDENTIFIER>
void RsTypeSerializer::print_data(
        const std::string& /*name*/,
        const t_RsGenericIdType<ID_SIZE_IN_BYTES,UPPER_CASE,UNIQUE_IDENTIFIER>& member )
{
	std::cerr << "  [RsGenericId<" << std::hex << UNIQUE_IDENTIFIER << ">] : "
	          << member << std::endl;
}

template<uint32_t ID_SIZE_IN_BYTES,bool UPPER_CASE,uint32_t UNIQUE_IDENTIFIER>
bool RsTypeSerializer::to_JSON( const std::string& memberName,
                                const t_RsGenericIdType<ID_SIZE_IN_BYTES,UPPER_CASE,UNIQUE_IDENTIFIER>& member,
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

template<uint32_t ID_SIZE_IN_BYTES,bool UPPER_CASE,uint32_t UNIQUE_IDENTIFIER>
bool RsTypeSerializer::from_JSON( const std::string& membername,
                                  t_RsGenericIdType<ID_SIZE_IN_BYTES,UPPER_CASE,UNIQUE_IDENTIFIER>& member,
                                  RsJson& jVal )
{
	const char* mName = membername.c_str();
	bool ret = jVal.HasMember(mName);
	if(ret)
	{
		rapidjson::Value& v = jVal[mName];
		ret = ret && v.IsString();
		ret && (member = t_RsGenericIdType<ID_SIZE_IN_BYTES,UPPER_CASE,UNIQUE_IDENTIFIER>(std::string(v.GetString())), false);
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
// Generic types                                                              //
//============================================================================//

template<typename T> /*static*/
bool RsTypeSerializer::to_JSON(const std::string& memberName, const T& member,
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

template<typename T> /*static*/
bool RsTypeSerializer::from_JSON( const std::string& memberName,
                                  T& member, RsJson& /*jDoc*/ )
{
	std::cerr << __PRETTY_FUNCTION__ << " JSON deserialization for type "
	          << typeid(member).name() << " " << memberName
	          << " not available." << std::endl;
	print_stacktrace();
	return true;
}
