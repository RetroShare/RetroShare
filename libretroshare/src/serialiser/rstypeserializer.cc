/*******************************************************************************
 * libretroshare/src/serialiser: rstypeserializer.cc                           *
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
#include "serialiser/rsserializer.h"
#include "serialiser/rstypeserializer.h"
#include "serialiser/rsbaseserial.h"
#include "serialiser/rstlvkeys.h"
#include "serialiser/rsserializable.h"
#include "util/rsbase64.h"
#include "util/rsprint.h"
#include "util/rstime.h"

#include <iomanip>
#include <string>
#include <typeinfo> // for typeid
#include <rapidjson/prettywriter.h>

#ifdef RSSERIAL_DEBUG
#	define SAFE_GET_JSON_V() \
	const char* mName = memberName.c_str(); \
	bool ret = jDoc.HasMember(mName); \
	if(!ret) \
    { \
	    std::cerr << __PRETTY_FUNCTION__ << " \"" << memberName \
	              << "\" not found in JSON:" << std::endl \
	              << jDoc << std::endl << std::endl; \
	    return false; \
	} \
	rapidjson::Value& v = jDoc[mName]
#else // ifdef RSSERIAL_DEBUG
#	define SAFE_GET_JSON_V() \
	const char* mName = memberName.c_str(); \
	bool ret = jDoc.HasMember(mName); \
	if(!ret) return false; \
	rapidjson::Value& v = jDoc[mName]
#endif // ifdef RSSERIAL_DEBUG

//============================================================================//
//                             std::string                                    //
//============================================================================//

template<>  /*static*/
bool RsTypeSerializer::to_JSON( const std::string& membername,
                                const std::string& member, RsJson& jDoc )
{
	rapidjson::Document::AllocatorType& allocator = jDoc.GetAllocator();

	rapidjson::Value key;
	key.SetString( membername.c_str(),
	               static_cast<rapidjson::SizeType>(membername.length()),
	               allocator );

	rapidjson::Value value;
	value.SetString( member.c_str(),
	                 static_cast<rapidjson::SizeType>(member.length()),
	                 allocator );

	jDoc.AddMember(key, value, allocator);

	return true;
}
template<> /*static*/
bool RsTypeSerializer::from_JSON( const std::string& memberName,
                                  std::string& member, RsJson& jDoc )
{
	SAFE_GET_JSON_V();
	ret = ret && v.IsString();
	if(ret) member = v.GetString();
	return ret;
}


//============================================================================//
//                            Integral types                                  //
//============================================================================//

#define SIMPLE_TO_JSON_DEF(T) \
template<> bool RsTypeSerializer::to_JSON( const std::string& memberName, \
	                                       const T& member, RsJson& jDoc ) \
{ \
	rapidjson::Document::AllocatorType& allocator = jDoc.GetAllocator(); \
	\
	rapidjson::Value key; \
	key.SetString( memberName.c_str(), \
	               static_cast<rapidjson::SizeType>(memberName.length()), \
	               allocator ); \
 \
	rapidjson::Value value(member); \
 \
	jDoc.AddMember(key, value, allocator); \
 \
	return true; \
}

SIMPLE_TO_JSON_DEF(bool)
SIMPLE_TO_JSON_DEF(int32_t)

SIMPLE_TO_JSON_DEF(uint8_t)
SIMPLE_TO_JSON_DEF(uint16_t)
SIMPLE_TO_JSON_DEF(uint32_t)

/** Be very careful in changing this constant as it would break 64 bit integers
 * members JSON string representation retrocompatibility */
static constexpr char strReprKey[] = "xstr64";

/** Be very careful in changing this constant as it would break 64 bit integers
 * members JSON string representation retrocompatibility */
static constexpr char intReprKey[] = "xint64";

/** While JSON doesn't have problems representing 64 bits integers JavaScript,
 * Dart and other languages represents numbers in a double-like format thus they
 * are not capable to handle safely integers outside the range
 * [-(2^53 - 1), 2^53 - 1].
 * To overcome this limitation we represent 64 bit integers as an object with
 * two keys, one as integer and one as string representation.
 * In our case we need to wrap those into an object instead of just adding a key
 * with a suffix so support well also containers like std::map or std::vector.
 * More discussion on the topic at @see https://stackoverflow.com/a/34989371
 */
#define SIXTYFOUR_INTEGERS_TO_JSON_DEF(T) \
template<> bool RsTypeSerializer::to_JSON( const std::string& memberName, \
	                                       const T& member, RsJson& jDoc ) \
{ \
	using namespace rapidjson; \
	Document::AllocatorType& allocator = jDoc.GetAllocator(); \
 \
	Document wrapper(rapidjson::kObjectType, &allocator); \
 \
	Value intKey; \
	intKey.SetString(intReprKey, allocator ); \
	Value intValue(member); \
	wrapper.AddMember(intKey, intValue, allocator); \
 \
	bool ok = to_JSON(strReprKey, std::to_string(member), wrapper); \
 \
	Value key; \
	key.SetString( memberName.c_str(), \
	               static_cast<rapidjson::SizeType>(memberName.length()), \
	               allocator ); \
	jDoc.AddMember(key, wrapper, allocator); \
 \
	return ok; \
}

SIXTYFOUR_INTEGERS_TO_JSON_DEF(int64_t);
SIXTYFOUR_INTEGERS_TO_JSON_DEF(uint64_t);

template<> /*static*/
bool RsTypeSerializer::from_JSON( const std::string& memberName, bool& member,
                                  RsJson& jDoc )
{
	SAFE_GET_JSON_V();
	ret = ret && v.IsBool();
	if(ret) member = v.GetBool();
	return ret;
}

template<> /*static*/
bool RsTypeSerializer::from_JSON( const std::string& memberName,
                                  int32_t& member, RsJson& jDoc )
{
	SAFE_GET_JSON_V();
	ret = ret && v.IsInt();
	if(ret) member = v.GetInt();
	return ret;
}

template<> /*static*/
bool RsTypeSerializer::from_JSON( const std::string& memberName,
                                  uint8_t& member, RsJson& jDoc )
{
	SAFE_GET_JSON_V();
	ret = ret && v.IsUint();
	if(ret) member = static_cast<uint8_t>(v.GetUint());
	return ret;
}

template<> /*static*/
bool RsTypeSerializer::from_JSON( const std::string& memberName,
                                  uint16_t& member, RsJson& jDoc )
{
	SAFE_GET_JSON_V();
	ret = ret && v.IsUint();
	if(ret) member = static_cast<uint16_t>(v.GetUint());
	return ret;
}

template<> /*static*/
bool RsTypeSerializer::from_JSON( const std::string& memberName,
                                  uint32_t& member, RsJson& jDoc )
{
	SAFE_GET_JSON_V();
	ret = ret && v.IsUint();
	if(ret) member = v.GetUint();
	return ret;
}

/** inverse of @see SIXTYFOUR_INTEGERS_TO_JSON_DEF */
#define SIXTYFOUR_INTEGERS_FROM_JSON_DEF(T, PRED, GET, CONV) \
template<> bool RsTypeSerializer::from_JSON( \
	    const std::string& memberName, T& member, RsJson& jDoc ) \
{ \
	using namespace rapidjson; \
 \
	SAFE_GET_JSON_V(); \
 \
	/* For retro-compatibility take it directly if it is passed as integer */ \
	if(v.PRED()) \
    { \
	    member = v.GET(); \
	    return true; \
	} \
 \
	ret = ret && v.IsObject(); \
 \
	if(!ret) \
    { \
	    Dbg3() << __PRETTY_FUNCTION__ << " " << memberName << " not found" \
	           << std::endl; \
	    return false; \
	} \
 \
	if(v.HasMember(intReprKey)) \
    { \
	    Value& iVal = v[intReprKey]; \
	    if(iVal.PRED()) \
        { \
	        member = iVal.GET(); \
	        return true; \
	    } \
	} \
 \
	Dbg4() << __PRETTY_FUNCTION__ << " integer representation of " << memberName \
	       << " not found in JSON then attempt to look for string representation" \
	       << std::endl; \
 \
 \
	if(v.HasMember(strReprKey)) \
    { \
	    Value& sVal = v[strReprKey]; \
	    if(sVal.IsString()) \
        { \
	        try { member = CONV(sVal.GetString()); } \
	        catch (...) \
            { \
	            RsErr() << __PRETTY_FUNCTION__ << " cannot convert " \
	                    << sVal.GetString() << " to integral type" << std::endl; \
	            return false; \
	        } \
 \
	        return true; \
	    } \
	} \
 \
	Dbg3() << __PRETTY_FUNCTION__ << " neither integral representation nor " \
	       << "string representation found for: " <<  memberName << std::endl; \
 \
	return false; \
}

SIXTYFOUR_INTEGERS_FROM_JSON_DEF(uint64_t, IsUint64, GetUint64, std::stoull)
SIXTYFOUR_INTEGERS_FROM_JSON_DEF( int64_t,  IsInt64,  GetInt64,  std::stoll)

//============================================================================//
//                                 Floats                                     //
//============================================================================//

template<> uint32_t RsTypeSerializer::serial_size(const float&){ return 4; }

template<> bool RsTypeSerializer::serialize(uint8_t data[], uint32_t size, uint32_t& offset, const float& f)
{
    return setRawUFloat32(data,size,&offset,f) ;
}
template<> bool RsTypeSerializer::deserialize(const uint8_t data[], uint32_t size, uint32_t &offset, float& f)
{
    return getRawUFloat32(data,size,&offset,f) ;
}
template<> void RsTypeSerializer::print_data(const std::string& n, const float& V)
{
    std::cerr << "  [float      ] " << n << ": " << V << std::endl;
}

SIMPLE_TO_JSON_DEF(float)

template<> /*static*/
bool RsTypeSerializer::from_JSON( const std::string& memberName,
                                  float& member, RsJson& jDoc )
{
	SAFE_GET_JSON_V();
	ret = ret && v.IsFloat();
	if(ret) member = v.GetFloat();
	return ret;
}

template<> /*static*/
uint32_t RsTypeSerializer::serial_size(const double&)
{
	std::cerr << "Binary [de]serialization not implemented yet for double"
	          << std::endl;
	print_stacktrace();
	return 0;
}

template<>  /*static*/
bool RsTypeSerializer::serialize(uint8_t[], uint32_t, uint32_t&, const double&)
{
	std::cerr << "Binary [de]serialization not implemented yet for double"
	          << std::endl;
	print_stacktrace();
	return false;
}

template<>  /*static*/
bool RsTypeSerializer::deserialize(const uint8_t[], uint32_t, uint32_t&, double&)
{
	std::cerr << "Binary [de]serialization not implemented yet for double"
	          << std::endl;
	print_stacktrace();
	return false;
}

template<>  /*static*/
void RsTypeSerializer::print_data(const std::string& n, const double& V)
{ std::cerr << "  [double     ] " << n << ": " << V << std::endl; }

SIMPLE_TO_JSON_DEF(double)

template<> /*static*/
bool RsTypeSerializer::from_JSON( const std::string& memberName,
                                  double& member, RsJson& jDoc )
{
	SAFE_GET_JSON_V();
	ret = ret && v.IsDouble();
	if(ret) member = v.GetDouble();
	return ret;
}


//============================================================================//
//                      TlvString with subtype                                //
//============================================================================//

template<> uint32_t RsTypeSerializer::serial_size(uint16_t /* type_subtype */,const std::string& s)
{ 
	return GetTlvStringSize(s) ;
}

template<> bool RsTypeSerializer::serialize(uint8_t data[], uint32_t size, uint32_t &offset,uint16_t type_substring,const std::string& s)
{
	return SetTlvString(data,size,&offset,type_substring,s) ;
}

template<> bool RsTypeSerializer::deserialize(const uint8_t data[], uint32_t size,uint32_t& offset,uint16_t type_substring,std::string& s)
{
	return GetTlvString((void*)data,size,&offset,type_substring,s) ;
}

template<> void RsTypeSerializer::print_data(const std::string& n, uint16_t type_substring,const std::string& V)
{
	std::cerr << "  [TlvString  ] " << n << ": type=" << std::hex <<std::setw(4)<<std::setfill('0') <<  type_substring << std::dec << " s=\"" << V<< "\"" << std::endl;
}

template<> /*static*/
bool RsTypeSerializer::to_JSON( const std::string& memberName,
                                uint16_t /*sub_type*/,
                                const std::string& member, RsJson& jDoc )
{
	return to_JSON<std::string>(memberName, member, jDoc);
}

template<> /*static*/
bool RsTypeSerializer::from_JSON( const std::string& memberName,
                                  uint16_t /*sub_type*/,
                                  std::string& member, RsJson& jDoc )
{
	return from_JSON<std::string>(memberName, member, jDoc);
}

//============================================================================//
//                          TlvInt with subtype                               //
//============================================================================//

template<> uint32_t RsTypeSerializer::serial_size( uint16_t /* type_subtype */,
                                                   const uint32_t& /*s*/ )
{
	return GetTlvUInt32Size();
}

template<> bool RsTypeSerializer::serialize( uint8_t data[], uint32_t size,
                                             uint32_t &offset,uint16_t sub_type,
                                             const uint32_t& s)
{
	return SetTlvUInt32(data,size,&offset,sub_type,s);
}

template<> bool RsTypeSerializer::deserialize( const uint8_t data[],
                                               uint32_t size, uint32_t& offset,
                                               uint16_t sub_type, uint32_t& s)
{
	return GetTlvUInt32((void*)data, size, &offset, sub_type, &s);
}

template<> void RsTypeSerializer::print_data(const std::string& n, uint16_t sub_type,const uint32_t& V)
{
	std::cerr << "  [TlvUInt32  ] " << n << ": type=" << std::hex
	          << std::setw(4) << std::setfill('0') << sub_type << std::dec
	          << " s=\"" << V << "\"" << std::endl;
}

template<> /*static*/
bool RsTypeSerializer::to_JSON( const std::string& memberName,
                                uint16_t /*sub_type*/,
                                const uint32_t& member, RsJson& jDoc )
{
	return to_JSON<uint32_t>(memberName, member, jDoc);
}

template<> /*static*/
bool RsTypeSerializer::from_JSON( const std::string& memberName,
                                  uint16_t /*sub_type*/,
                                  uint32_t& member, RsJson& jDoc )
{
	return from_JSON<uint32_t>(memberName, member, jDoc);
}


//============================================================================//
//                              TlvItems                                      //
//============================================================================//

template<> uint32_t RsTypeSerializer::serial_size(const RsTlvItem& s)
{
	return s.TlvSize();
}

template<> bool RsTypeSerializer::serialize(uint8_t data[], uint32_t size,
                                            uint32_t &offset,const RsTlvItem& s)
{
	return s.SetTlv(data,size,&offset);
}

template<> bool RsTypeSerializer::deserialize(const uint8_t data[],
                                              uint32_t size,uint32_t& offset,
                                              RsTlvItem& s)
{
	return s.GetTlv((void*)data,size,&offset) ;
}

template<> void RsTypeSerializer::print_data( const std::string& n,
                                              const RsTlvItem& s )
{
	std::cerr << "  [" << typeid(s).name() << "] " << n << std::endl;
}

template<> /*static*/
bool RsTypeSerializer::to_JSON( const std::string& memberName,
                                const RsTlvItem& member, RsJson& jDoc )
{
	rapidjson::Document::AllocatorType& allocator = jDoc.GetAllocator();

	rapidjson::Value key;
	key.SetString(memberName.c_str(), memberName.length(), allocator);

	rapidjson::Value value;
	const char* tName = typeid(member).name();
	value.SetString(tName, allocator);

	jDoc.AddMember(key, value, allocator);

	return true;
}

template<> /*static*/
bool RsTypeSerializer::from_JSON( const std::string& /*memberName*/,
                                  RsTlvItem& member, RsJson& /*jDoc*/)
{
	member.TlvClear();
	return true;
}


//============================================================================//
//                              Binary blocks                                 //
//============================================================================//

/*static*/ /* without this Android compilation breaks */
constexpr uint32_t RsTypeSerializer::RawMemoryWrapper::MAX_SERIALIZED_CHUNK_SIZE;

/*static*/
void RsTypeSerializer::RawMemoryWrapper::serial_process(
                RsGenericSerializer::SerializeJob j,
                RsGenericSerializer::SerializeContext& ctx )
{
	switch(j)
	{
	case RsGenericSerializer::SIZE_ESTIMATE:
		RS_SERIAL_PROCESS(second);
		ctx.mOffset += second;
		break;
	case RsGenericSerializer::SERIALIZE:
		if(!ctx.mOk) break;
		if(second > MAX_SERIALIZED_CHUNK_SIZE)
		{
			RsErr() << __PRETTY_FUNCTION__
			        << std::errc::message_size << " "
			        << second << " > " << MAX_SERIALIZED_CHUNK_SIZE
			        << std::endl;
			print_stacktrace();
			break;
		}
		RS_SERIAL_PROCESS(second);
		if(!ctx.mOk) break;
		ctx.mOk = ctx.mSize >= ctx.mOffset + second;
		if(!ctx.mOk)
		{
			RsErr() << __PRETTY_FUNCTION__ << std::errc::no_buffer_space
			        << std::endl;
			print_stacktrace();
			break;
		}
		memcpy(ctx.mData + ctx.mOffset, first, second);
		ctx.mOffset += second;
		break;
	case RsGenericSerializer::DESERIALIZE:
		if(first || second)
		{
			/* Items are created anew before deserialization so buffer pointer
			 * must be null and size 0 at this point */

			RsWarn() << __PRETTY_FUNCTION__ << " DESERIALIZE got uninitialized "
			         << " or pre-allocated buffer! Buffer pointer: " << first
			         << " must be null and size: " << second << " must be 0 at "
			         << "this point. Does your item costructor initialize them "
			         << "properly?" << std::endl;
			print_stacktrace();
		}

		RS_SERIAL_PROCESS(second);
		if(!ctx.mOk) break;
		ctx.mOk = (second <= MAX_SERIALIZED_CHUNK_SIZE);
		if(!ctx.mOk)
		{
			RsErr() << __PRETTY_FUNCTION__
			        << std::errc::message_size << " "
			        << second << " > " << MAX_SERIALIZED_CHUNK_SIZE
			        << std::endl;
			clear();
			break;
		}

		if(!second)
		{
			Dbg3() << __PRETTY_FUNCTION__ << " Deserialized empty memory chunk"
			       << std::endl;
			clear();
			break;
		}

		ctx.mOk = ctx.mSize >= ctx.mOffset + second;
		if(!ctx.mOk)
		{
			RsErr() << __PRETTY_FUNCTION__ << std::errc::no_buffer_space
			        << std::endl;
			print_stacktrace();

			clear();
			break;
		}

		first = reinterpret_cast<uint8_t*>(malloc(second));
		memcpy(first, ctx.mData + ctx.mOffset, second);
		ctx.mOffset += second;
		break;
	case RsGenericSerializer::PRINT:  break;
	case RsGenericSerializer::TO_JSON:
	{
		if(!ctx.mOk) break;
		std::string encodedValue;
		RsBase64::encode(first, second, encodedValue, true, false);
		ctx.mJson.SetString(
		            encodedValue.data(),
		            static_cast<rapidjson::SizeType>(encodedValue.length()),
		            ctx.mJson.GetAllocator());
		break;
	}
	case RsGenericSerializer::FROM_JSON:
	{
		const bool yelding = !!(
		            RsSerializationFlags::YIELDING & ctx.mFlags );
		if(!(ctx.mOk || yelding))
		{
			clear();
			break;
		}
		if(!ctx.mJson.IsString())
		{
			RsErr() << __PRETTY_FUNCTION__ << " "
			        << std::errc::invalid_argument << std::endl;
			print_stacktrace();

			ctx.mOk = false;
			clear();
			break;
		}
		if( ctx.mJson.GetStringLength() >
		        RsBase64::encodedSize(MAX_SERIALIZED_CHUNK_SIZE, true) )
		{
			RsErr() << __PRETTY_FUNCTION__ << " "
			        << std::errc::message_size << std::endl;
			print_stacktrace();

			ctx.mOk = false;
			clear();
			break;
		}

		std::string encodedValue = ctx.mJson.GetString();
		std::vector<uint8_t> decoded;
		auto ec = RsBase64::decode(encodedValue, decoded);
		if(ec)
		{
			RsErr() << __PRETTY_FUNCTION__ << " " << ec << std::endl;
			print_stacktrace();

			ctx.mOk = false;
			clear();
			break;
		}

		const auto decodedSize = decoded.size();

		if(!decodedSize)
		{
			clear();
			break;
		}

		if(decodedSize != second)
		{
			first = reinterpret_cast<uint8_t*>(realloc(first, decodedSize));
			second = static_cast<uint32_t>(decodedSize);
		}

		memcpy(first, decoded.data(), second);
		break;
	}
	default: RsTypeSerializer::fatalUnknownSerialJob(j);
	}
}

void RsTypeSerializer::RawMemoryWrapper::clear()
{
	free(first);
	first = nullptr;
	second = 0;
}

//============================================================================//
//                         std::error_condition                               //
//============================================================================//

void RsTypeSerializer::ErrConditionWrapper::serial_process(
        RsGenericSerializer::SerializeJob j,
        RsGenericSerializer::SerializeContext& ctx )
{
	switch(j)
	{
	case RsGenericSerializer::SIZE_ESTIMATE: // fallthrough
	case RsGenericSerializer::DESERIALIZE: // fallthrough
	case RsGenericSerializer::SERIALIZE: // fallthrough
	case RsGenericSerializer::FROM_JSON: // [[fallthrough]]
	case RsGenericSerializer::PRINT:
		RsFatal() << __PRETTY_FUNCTION__ << " SerializeJob: " << j
		          << "is not supported on std::error_condition " << std::endl;
		print_stacktrace();
		exit(-2);
	case RsGenericSerializer::TO_JSON:
	{
		constexpr RsGenericSerializer::SerializeJob rj =
		        RsGenericSerializer::TO_JSON;

		int32_t tNum = mec.value();
		RsTypeSerializer::serial_process(rj, ctx, tNum, "errorNumber");

		std::string tStr = mec.category().name();
		RsTypeSerializer::serial_process(rj, ctx, tStr, "errorCategory");

		tStr = mec.message();
		RsTypeSerializer::serial_process(rj, ctx, tStr, "errorMessage");
		break;
	}
	default: RsTypeSerializer::fatalUnknownSerialJob(j);
	}
}
