/*******************************************************************************
 * libretroshare/src/serialiser: rstlvgenericmap.inl                           *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2014 by Robert Fernie <retroshare@lunamutt.com>                   *
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

#include "serialiser/rstlvbase.h"

// Must be different - as this is inline in headers.
//#define TLV_GENERICMAP_DEBUG 1

/*********************************** RsTlvGenericPairRef ***********************************/

template<class K, class V>
void RsTlvGenericPairRef<K, V>::TlvClear()
{
	RsTlvParamRef<K> key(mKeyType, mKey);
	RsTlvParamRef<V> value(mValueType, mValue);
	key.TlvClear();
	value.TlvClear();
}

template<class K, class V>
uint32_t RsTlvGenericPairRef<K, V>::TlvSize() const
{
	uint32_t s = TLV_HEADER_SIZE; /* header */
	RsTlvParamRef<K> key(mKeyType, mKey);
	RsTlvParamRef<V> value(mValueType, mValue);

	s += key.TlvSize();
	s += value.TlvSize();
	return s;

}

template<class K, class V>
bool  RsTlvGenericPairRef<K, V>::SetTlv(void *data, uint32_t size, uint32_t *offset) const
{
	/* must check sizes */
	uint32_t tlvsize = TlvSize();
	uint32_t tlvend  = *offset + tlvsize;

	if (size < tlvend)
		return false; /* not enough space */

	bool ok = true;

	/* start at data[offset] */
	ok &= SetTlvBase(data, tlvend, offset, mPairType, tlvsize);
	
	RsTlvParamRef<K> key(mKeyType, mKey);
	RsTlvParamRef<V> value(mValueType, mValue);
	ok &= key.SetTlv(data, tlvend, offset);
	ok &= value.SetTlv(data, tlvend, offset);

	if (!ok)
	{
		std::cerr << "RsTlvGenericPairRef<>::SetTlv() Failed";
		std::cerr << std::endl;
	}
#ifdef TLV_GENERICMAP_DEBUG
    else
    {
        std::cerr << "RsTlvGenericPairRef<>::SetTlv() Ok";
        std::cerr << std::endl;
	}
#endif
    return ok;
}

template<class K, class V>
bool  RsTlvGenericPairRef<K, V>::GetTlv(void *data, uint32_t size, uint32_t *offset) 
{
	if (size < *offset + TLV_HEADER_SIZE)
		return false;	
	
	uint16_t tlvtype = GetTlvType( &(((uint8_t *) data)[*offset])  );
	uint32_t tlvsize = GetTlvSize( &(((uint8_t *) data)[*offset])  );
	uint32_t tlvend = *offset + tlvsize;

	if (size < tlvend)    /* check size */
		return false; /* not enough space */

	if (tlvtype != mPairType) /* check type */
		return false;

	bool ok = true;

	/* ready to load */
	TlvClear();

	/* skip the header */
	(*offset) += TLV_HEADER_SIZE;

	RsTlvParamRef<K> key(mKeyType, mKey);
	RsTlvParamRef<V> value(mValueType, mValue);
	ok &= key.GetTlv(data, tlvend, offset);
	ok &= value.GetTlv(data, tlvend, offset);

	/***************************************************************************
	 * NB: extra components could be added (for future expansion of the type).
	 *            or be present (if this code is reading an extended version).
	 *
	 * We must chew up the extra characters to conform with TLV specifications
	 ***************************************************************************/
	if (*offset != tlvend)
	{
#ifdef TLV_GENERICMAP_DEBUG
		std::cerr << "RsTlvGenericPairRef::GetTlv() Warning extra bytes at end of item";
		std::cerr << std::endl;
#endif
		*offset = tlvend;
	}
	return ok;
	
}

template<class K, class V>
std::ostream &RsTlvGenericPairRef<K, V>::print(std::ostream &out, uint16_t indent) const
{ 
	//printBase(out, "RsTlvGenericPairRef", indent);
	uint16_t int_Indent = indent + 2;

	RsTlvParamRef<K> key(mKeyType, mKey);
	RsTlvParamRef<V> value(mValueType, mValue);

	printIndent(out, int_Indent);
	out << "Key:";
	key.print(out, 0);
	out << std::endl;

	printIndent(out, int_Indent);
	out << "Value:";
	value.print(out, 0);
	out << std::endl;
	
	//printEnd(out, "RsTlvGenericPairRef", indent);
	return out;
}

// declare likely combinations.
//template<> class RsTlvGenericPairRef<uint32_t, uint32_t>;
//template<> class RsTlvGenericPairRef<uint32_t, std::string>;
//template<> class RsTlvGenericPairRef<std::string, uint32_t>;
//template<> class RsTlvGenericPairRef<std::string, std::string>;

/************************************ RsTlvGenericMapRef ***********************************/

template<class K, class V>
void RsTlvGenericMapRef<K, V>::TlvClear()
{
	mRefMap.clear(); //empty list
}

template<class K, class V>
uint32_t RsTlvGenericMapRef<K, V>::TlvSize() const
{
	uint32_t s = TLV_HEADER_SIZE; /* header */

	typename std::map<K, V>::iterator it;
	for(it = mRefMap.begin(); it != mRefMap.end(); ++it)
	{
		RsTlvGenericPairRef<const K, V> pair(mPairType, mKeyType, mValueType, it->first, it->second);
		s += pair.TlvSize();
	}

	return s;
}

template<class K, class V>
bool  RsTlvGenericMapRef<K, V>::SetTlv(void *data, uint32_t size, uint32_t *offset) const
{
	/* must check sizes */
	uint32_t tlvsize = TlvSize();
	uint32_t tlvend  = *offset + tlvsize;

	if (size < tlvend)
		return false; /* not enough space */

	bool ok = true;

	/* start at data[offset] */
	ok &= SetTlvBase(data, tlvend, offset, mMapType, tlvsize);
	

	typename std::map<K, V>::iterator it;
	for(it = mRefMap.begin(); it != mRefMap.end(); ++it)
	{
		RsTlvGenericPairRef<const K, V> pair(mPairType, mKeyType, mValueType, it->first, it->second);
		ok &= pair.SetTlv(data, size, offset);
	}

	if (!ok)
	{
		std::cerr << "RsTlvGenericMapRef<>::SetTlv() Failed";
		std::cerr << std::endl;
	}

	return ok;
}

template<class K, class V>
bool  RsTlvGenericMapRef<K, V>::GetTlv(void *data, uint32_t size, uint32_t *offset) 
{
	if (size < *offset + TLV_HEADER_SIZE)
		return false;	

	uint16_t tlvtype = GetTlvType( &(((uint8_t *) data)[*offset])  );
	uint32_t tlvsize = GetTlvSize( &(((uint8_t *) data)[*offset])  );
	uint32_t tlvend = *offset + tlvsize;

	if (size < tlvend)    /* check size */
		return false; /* not enough space */

	if (tlvtype != mMapType) /* check type */
		return false;

	bool ok = true;

	/* ready to load */
	TlvClear();

	/* skip the header */
	(*offset) += TLV_HEADER_SIZE;

        /* while there is TLV  */
        while((*offset) + 2 < tlvend)
        {
                /* get the next type */
                uint16_t tlvsubtype = GetTlvType( &(((uint8_t *) data)[*offset]) );

		if (tlvsubtype == mPairType)
		{
			K k;
			V v;
			RsTlvGenericPairRef<K, V> pair(mPairType, mKeyType, mValueType, k, v);
			ok &= pair.GetTlv(data, size, offset);
			if (ok)
			{
				mRefMap[k] = v;
			}
		}
		else
		{
			ok &= SkipUnknownTlv(data, tlvend, offset);
			ok = false;
		}

                if (!ok)
			break;
	}
		
	/***************************************************************************
	 * NB: extra components could be added (for future expansion of the type).
	 *            or be present (if this code is reading an extended version).
	 *
	 * We must chew up the extra characters to conform with TLV specifications
	 ***************************************************************************/
	if (*offset != tlvend)
	{
#ifdef TLV_GENERICMAP_DEBUG
		std::cerr << "RsTlvGenericMapRef::GetTlv() Warning extra bytes at end of item";
		std::cerr << std::endl;
#endif
		*offset = tlvend;
	}

	return ok;
}

template<class K, class V>
std::ostream &RsTlvGenericMapRef<K, V>::print(std::ostream &out, uint16_t indent) const
{
	//printBase(out, "RsTlvGenericMapRef", indent);
	uint16_t int_Indent = indent + 2;
	
	printIndent(out, int_Indent);
	out << "MapType:" << mMapType << std::endl;

	typename std::map<K, V>::iterator it;
	for(it = mRefMap.begin(); it != mRefMap.end() ; ++it)
	{
		RsTlvGenericPairRef<const K, V> pair(mPairType, mKeyType, mValueType, it->first, it->second);
		pair.print(out, int_Indent);
	}

	//printEnd(out, "RsTlvGenericMapRef", indent);
	return out;
}

