/*******************************************************************************
 * libretroshare/src/serialiser: rstlvgenericparam.cc                          *
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
#include "serialiser/rstlvgenericparam.h"
#include "serialiser/rstlvbase.h"
#include <iostream>

// #define TLV_DEBUG 1

/* generic print */
template<class T>
std::ostream & RsTlvParamRef<T>::print(std::ostream &out, uint16_t indent) const
{
	printIndent(out, indent);
	out << "Type: " << mParamType << " Param: " << mParam;
	return out;
}


/***** uint16_t ****/
template<>
uint32_t RsTlvParamRef<uint16_t>::TlvSize() const
{
	return GetTlvUInt16Size();
}

template<>
void RsTlvParamRef<uint16_t>::TlvClear()
{
	mParam = 0;
}

template<>
bool RsTlvParamRef<uint16_t>::SetTlv(void *data, uint32_t size, uint32_t *offset) const
{
	return SetTlvUInt16(data, size, offset, mParamType, mParam);
}

template<>
bool RsTlvParamRef<uint16_t>::GetTlv(void *data, uint32_t size, uint32_t *offset)
{
	uint16_t param;
	bool retval = GetTlvUInt16(data, size, offset, mParamType, &param);
	mParam = param;
	return retval;
}

template<>
std::ostream & RsTlvParamRef<uint16_t>::print(std::ostream &out, uint16_t indent) const
{
	printIndent(out, indent);
	out << "Type: " << mParamType << "Param: " << mParam;
	return out;
}


/***** const uint16_t ****/

template<>
uint32_t RsTlvParamRef<const uint16_t>::TlvSize() const
{
	return GetTlvUInt16Size();
}

template<>
void RsTlvParamRef<const uint16_t>::TlvClear()
{
	//mParam = 0;
}


template<>
bool RsTlvParamRef<const uint16_t>::SetTlv(void *data, uint32_t size, uint32_t *offset) const
{
        return SetTlvUInt16(data, size, offset, mParamType, mParam);
}

template<>
bool RsTlvParamRef<const uint16_t>::GetTlv(void */*data*/, uint32_t /*size*/, uint32_t */*offset*/)
{
	return false; //GetTlvUInt16(data, size, offset, mParamType, &mParam);
}


template<>
std::ostream & RsTlvParamRef<const uint16_t>::print(std::ostream &out, uint16_t indent) const
{
	printIndent(out, indent);
	out << "Type: " << mParamType << "Param: " << mParam;
	return out;
}

/***** uint32_t ****/
template<>
uint32_t RsTlvParamRef<uint32_t>::TlvSize() const
{
	return GetTlvUInt32Size();
}

template<>
void RsTlvParamRef<uint32_t>::TlvClear()
{
	mParam = 0;
}

template<>
bool RsTlvParamRef<uint32_t>::SetTlv(void *data, uint32_t size, uint32_t *offset) const
{
        return SetTlvUInt32(data, size, offset, mParamType, mParam);
}

template<>
bool RsTlvParamRef<uint32_t>::GetTlv(void *data, uint32_t size, uint32_t *offset)
{
	return GetTlvUInt32(data, size, offset, mParamType, &mParam);
}

template<>
std::ostream & RsTlvParamRef<uint32_t>::print(std::ostream &out, uint16_t indent) const
{
	printIndent(out, indent);
	out << "Type: " << mParamType << "Param: " << mParam;
	return out;
}



/***** const uint32_t ****/
template<>
uint32_t RsTlvParamRef<const uint32_t>::TlvSize() const
{
	return GetTlvUInt32Size();
}

template<>
void RsTlvParamRef<const uint32_t>::TlvClear()
{
	//mParam = 0;
}

template<>
bool RsTlvParamRef<const uint32_t>::SetTlv(void *data, uint32_t size, uint32_t *offset) const
{
        return SetTlvUInt32(data, size, offset, mParamType, mParam);
}

template<>
bool RsTlvParamRef<const uint32_t>::GetTlv(void */*data*/, uint32_t /*size*/, uint32_t */*offset*/)
{
	return false;
	//GetTlvUInt32(data, size, offset, mParamType, &mParam);
}

template<>
std::ostream & RsTlvParamRef<const uint32_t>::print(std::ostream &out, uint16_t indent) const
{
	printIndent(out, indent);
	out << "Type: " << mParamType << "Param: " << mParam;
	return out;
}


/***** std::string ****/
template<>
uint32_t RsTlvParamRef<std::string>::TlvSize() const
{
	return GetTlvStringSize(mParam);
}

template<>
void RsTlvParamRef<std::string>::TlvClear()
{
	mParam.clear();
}

template<>
bool RsTlvParamRef<std::string>::SetTlv(void *data, uint32_t size, uint32_t *offset) const
{
        return SetTlvString(data, size, offset, mParamType, mParam);
}

template<>
bool RsTlvParamRef<std::string>::GetTlv(void *data, uint32_t size, uint32_t *offset)
{
	return GetTlvString(data, size, offset, mParamType, mParam);
}

template<>
std::ostream & RsTlvParamRef<std::string>::print(std::ostream &out, uint16_t indent) const
{
	printIndent(out, indent);
	out << "Type: " << mParamType << "Param: " << mParam;
	return out;
}

// declare likely combinations.
//template class RsTlvParamRef<uint16_t>;
//template class RsTlvParamRef<uint32_t>;
//template class RsTlvParamRef<std::string>;

//template class RsTlvParamRef<const uint16_t>;
//template class RsTlvParamRef<const uint32_t>;
//template class RsTlvParamRef<const std::string>;

