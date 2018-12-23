/*******************************************************************************
 * libresapi/api/jsonStream.cpp                                                *
 *                                                                             *
 * LibResAPI: API for local socket server                                      *
 *                                                                             *
 * Copyright 2018 by Retroshare Team <retroshare.project@gmail.com>            *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Affero General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/
#include "JsonStream.h"
#include <iostream>

namespace resource_api
{

JsonStream::JsonStream():
    mSerialise(true), mDataType(TYPE_UNDEFINED), mArrayNextRead(0), mIsOk(true), mChild(NULL)
{

}

JsonStream::~JsonStream()
{
    deleteCurrentChild();
}

void JsonStream::setJsonString(std::string jsonStr)
{
    mRawString = jsonStr;
    // have to delay the deserialisation, because this stream can also be a raw data stream without json
    // can find this out when others atucally call the operators
    mSerialise = false;
}

std::string JsonStream::getJsonString()
{
    deleteCurrentChild();
    if(mIsOk)
    {
        switch(mDataType)
        {
        case TYPE_UNDEFINED:
            return "";
        case TYPE_ARRAY:
            return json::Serialize(mArray);
        case TYPE_OBJECT:
            return json::Serialize(mObject);
        case TYPE_RAW:
            return mRawString;
        default:
            return "";
        }
    }
    std::cerr << "JsonStream::getJsonString() Warning: stream not ok, will return empty string." << std::endl;
    return "";
}

void JsonStream::switchToDeserialisation()
{
    deleteCurrentChild();
    mSerialise = false;
}


//----------Stream Interface ---------------

//----------Array---------------
StreamBase& JsonStream::operator<<(ValueReference<bool> value)
{
    if(serialise())
    {
        setType(TYPE_ARRAY);
        mArray.push_back(value.value);
    }
    else
    {
        if(checkDeserialisation() && arrayBoundsOk())
        {
            valueToBool(mArray[mArrayNextRead], value.value);
            mArrayNextRead++;
        }
    }
    return *this;
}

StreamBase& JsonStream::operator<<(ValueReference<int> value)
{
    if(serialise())
    {
        setType(TYPE_ARRAY);
        mArray.push_back(value.value);
    }
    else
    {
        if(checkDeserialisation() && arrayBoundsOk())
        {
            valueToInt(mArray[mArrayNextRead], value.value);
            mArrayNextRead++;
        }
    }
    return *this;
}

StreamBase& JsonStream::operator<<(ValueReference<double> value)
{
    if(serialise())
    {
        setType(TYPE_ARRAY);
        mArray.push_back(value.value);
    }
    else
    {
        if(checkDeserialisation() && arrayBoundsOk())
        {
            valueToDouble(mArray[mArrayNextRead], value.value);
            mArrayNextRead++;
        }
    }
    return *this;
}

StreamBase& JsonStream::operator<<(ValueReference<std::string> value)
{
    if(serialise())
    {
        setType(TYPE_ARRAY);
        mArray.push_back(value.value);
    }
    else
    {
        if(checkDeserialisation() && arrayBoundsOk())
        {
            valueToString(mArray[mArrayNextRead], value.value);
            mArrayNextRead++;
        }
    }
    return *this;
}

StreamBase& JsonStream::getStreamToMember()
{
    setType(TYPE_ARRAY);
    deleteCurrentChild();
    mChild = new JsonStream();
    if(!serialise())
    {
        if(checkDeserialisation() && arrayBoundsOk())
        {
            mChild->mValue = mArray[mArrayNextRead];
            mChild->mSerialise = false;
            mArrayNextRead++;
        }
    }
    return *mChild;
}

//----------Object---------------
StreamBase& JsonStream::operator<<(KeyValueReference<bool> keyValue)
{
    if(serialise())
    {
        setType(TYPE_OBJECT);
        mObject[keyValue.key] = keyValue.value;
    }
    else
    {
        if(checkDeserialisation() && checkObjectMember(keyValue.key))
        {
            valueToBool(mObject[keyValue.key], keyValue.value);
        }
    }
    return *this;
}

StreamBase& JsonStream::operator<<(KeyValueReference<int> keyValue)
{
    if(serialise())
    {
        setType(TYPE_OBJECT);
        mObject[keyValue.key] = keyValue.value;
    }
    else
    {
        if(checkDeserialisation() && checkObjectMember(keyValue.key))
        {
            valueToInt(mObject[keyValue.key], keyValue.value);
        }
    }
    return *this;
}

StreamBase& JsonStream::operator<<(KeyValueReference<double> keyValue)
{
    if(serialise())
    {
        setType(TYPE_OBJECT);
        mObject[keyValue.key] = keyValue.value;
    }
    else
    {
        if(checkDeserialisation() && checkObjectMember(keyValue.key))
        {
            valueToDouble(mObject[keyValue.key], keyValue.value);
        }
    }
    return *this;
}

StreamBase& JsonStream::operator<<(KeyValueReference<std::string> keyValue)
{
    if(serialise())
    {
        setType(TYPE_OBJECT);
        mObject[keyValue.key] = keyValue.value;
    }
    else
    {
        if(checkDeserialisation() && checkObjectMember(keyValue.key))
        {
            valueToString(mObject[keyValue.key], keyValue.value);
        }
    }
    return *this;
}

// usefull if the new object member should be an array or object
// the reference should be at least valid until another method of this class gets called
StreamBase& JsonStream::getStreamToMember(std::string name)
{
    setType(TYPE_OBJECT);
    deleteCurrentChild();
    mChildKey = name;
    mChild = new JsonStream();
    if(!serialise())
    {
        mChild->mSerialise = false;
        if(checkDeserialisation() && checkObjectMember(name))
        {
            mChild->mValue = mObject[name];
        }
    }
    return *mChild;
}

// make a binay data object (not a real object, just binary data)
StreamBase& JsonStream::operator<<(std::vector<uint8_t>& data)
{
    if(serialise())
    {
        if((mDataType == TYPE_UNDEFINED)||(mDataType == TYPE_RAW))
        {
            mDataType = TYPE_RAW;
            mRawString = std::string(data.begin(), data.end());
        }
        else
        {
            mErrorLog += "Error: trying to set raw data while the type of this object is already another type\n";
            mIsOk = false;
        }
    }
    else
    {
        if((mDataType == TYPE_UNDEFINED)||(mDataType == TYPE_RAW))
        {
            mDataType = TYPE_RAW;
            data = std::vector<uint8_t>(mRawString.begin(), mRawString.end());
        }
        else
        {
            mErrorLog += "Error: trying to read raw data while the type of this object is already another type\n";
            mIsOk = false;
        }
    }
    return *this;
}

// return true if there are more members in this object/array
// useful for array reading
bool JsonStream::hasMore()
{
    return arrayBoundsOk();
}

bool JsonStream::serialise()
{
    return mSerialise;
}

bool JsonStream::isOK()
{
    return mIsOk;
}

void JsonStream::setError()
{
    mIsOk = false;
}

/*
void JsonStream::addLogMsg(std::string msg)
{}
*/

void JsonStream::addErrorMsg(std::string msg)
{
    mErrorLog += msg;
}

std::string JsonStream::getLog()
{
    return "not implemented yet";
}

std::string JsonStream::getErrorLog()
{
    return mErrorLog;
}

bool JsonStream::isRawData()
{
    return mDataType == TYPE_RAW;
}

std::string JsonStream::getRawData()
{
    return mRawString;
}

void JsonStream::setType(DataType type)
{
    if((mDataType == TYPE_UNDEFINED)||(mDataType == type))
    {
        mDataType = type;
    }
    else
    {
        mIsOk = false;
        mErrorLog += "JsonStream::setType() Error: type alread set to another type\n";
    }
}

bool JsonStream::checkObjectMember(std::string key)
{
    if(mDataType == TYPE_OBJECT)
    {
        if(mObject.HasKey(key))
        {
            return true;
        }
        else
        {
            mErrorLog += "JsonStream::checkObjectMember() Warning: missing key \""+key+"\"\n";
            return false;
        }
    }
    else
    {
        mIsOk = false;
        mErrorLog += "JsonStream::checkObjectMember() Error: type is not TYPE_OBJECT\n";
        return false;
    }
}

bool JsonStream::arrayBoundsOk()
{
    if(checkDeserialisation())
    {
        if(mDataType == TYPE_ARRAY)
        {
            if(mArrayNextRead < mArray.size())
            {
                return true;
            }
            else
            {
                return false;
            }
        }
        else
        {
            mIsOk = false;
            mErrorLog += "JsonStream::arrayBoundsOk() Error: type is not TYPE_ARRAY\n";
            return false;
        }
    }
    return false;
}

bool JsonStream::checkDeserialisation()
{
    if(mDataType != TYPE_RAW)
    {
        if(mDataType == TYPE_UNDEFINED)
        {
            if((mValue.GetType() == json::NULLVal) && mRawString != "")
            {
                mValue = json::Deserialize(mRawString);
            }
            if(mValue.GetType() == json::ObjectVal)
            {
                mDataType = TYPE_OBJECT;
                mObject = mValue;
                return true;
            }
            else if(mValue.GetType() == json::ArrayVal)
            {
                mDataType = TYPE_ARRAY;
                mArray = mValue;
                return true;
            }
            else
            {
                mIsOk = false;
                mErrorLog += "JsonStream::checkDeserialisation() Error: deserialisation did not end with an object or array\n";
                return false;
            }
        }
        else
        {
            // already deserialised
            return true;
        }
    }
    else
    {
        mIsOk = false;
        mErrorLog += "JsonStream::checkDeserialisation() Error: type is TYPE_RAW\n";
        return false;
    }
}

void JsonStream::valueToBool(json::Value &value, bool &boolean)
{
    if(value.GetType() == json::BoolVal)
    {
        boolean = value;
    }
    else
    {
        mIsOk = false;
        mErrorLog += "JsonStream::valueToBool() Error: wrong type\n";
    }
}

void JsonStream::valueToInt(json::Value &value, int &integer)
{
    if(value.GetType() == json::IntVal)
    {
        integer = value;
    }
    else
    {
        mIsOk = false;
        mErrorLog += "JsonStream::valueToInt() Error: wrong type\n";
    }
}

void JsonStream::valueToDouble(json::Value &value, double &doubleVal)
{
    if(value.IsNumeric())
    {
        doubleVal = value;
    }
    else
    {
        mIsOk = false;
        mErrorLog += "JsonStream::valueToDouble() Error: wrong type\n";
    }
}

void JsonStream::valueToString(json::Value &value, std::string& str)
{
    if(value.GetType() == json::StringVal)
    {
        str = value.ToString();
    }
    else
    {
        mIsOk = false;
        mErrorLog += "JsonStream::valueToString() Error: wrong type\n";
    }
}

void JsonStream::deleteCurrentChild()
{
    if(mChild)
    {
        if(serialise())
        {
            if(mDataType == TYPE_ARRAY)
            {
                // don't add empty value
                if(mChild->getJsonValue().GetType() != json::NULLVal)
                    mArray.push_back(mChild->getJsonValue());
            }
            else if(mDataType == TYPE_OBJECT)
            {
                mObject[mChildKey] = mChild->getJsonValue();
            }
            else
            {
                mErrorLog += "JsonStream::deleteCurrentChild() Error: cannot add child because own type is wrong\n";
            }
        }
        else
        {
            // don't have to do anything for deserialisation
        }
        delete mChild;
        mChild = NULL;
    }
}

json::Value JsonStream::getJsonValue()
{
    // remove the child and add it to own data
    deleteCurrentChild();
    switch(mDataType)
    {
    case TYPE_UNDEFINED:
        return json::Value();
    case TYPE_ARRAY:
        return mArray;
    case TYPE_OBJECT:
        return mObject;
    case TYPE_RAW:
        return mRawString;
    default:
        return json::Value();
    }
}

} // namespace resource_api
