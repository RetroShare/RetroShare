/*******************************************************************************
 * libresapi/api/jsonStream.h                                                  *
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
#pragma once

#include "ApiTypes.h"
#include "json.h"

namespace resource_api
{

class JsonStream: public StreamBase
{
public:
    JsonStream();
    virtual ~JsonStream();

    void setJsonString(std::string jsonStr);
    std::string getJsonString();

    // it is possible to use this class as buffer
    // first use as serialiser and fill with values
    // then call this method to deserialise the values
    void switchToDeserialisation();


    //----------Stream Interface ---------------

    // make an array
    virtual StreamBase& operator<<(ValueReference<bool> value);
    virtual StreamBase& operator<<(ValueReference<int> value);
    virtual StreamBase& operator<<(ValueReference<double> value);
    virtual StreamBase& operator<<(ValueReference<std::string> value);
    // usefull if the new array member should be an array or object
    // the reference should be at least valid until another method of this class gets called
    virtual StreamBase& getStreamToMember();

    // make an object
    virtual StreamBase& operator<<(KeyValueReference<bool> keyValue);
    virtual StreamBase& operator<<(KeyValueReference<int> keyValue);
    virtual StreamBase& operator<<(KeyValueReference<double> keyValue);
    virtual StreamBase& operator<<(KeyValueReference<std::string> keyValue);
    // usefull if the new object member should be an array or object
    // the reference should be at least valid until another method of this class gets called
    virtual StreamBase& getStreamToMember(std::string name);

    // make a binay data object (not a real object, just binary data)
    // idea: can use vector.swap() to allow passing larger data items without copying
    virtual StreamBase& operator<<(std::vector<uint8_t>& data);

    // return true if there are more members in this object/array
    // useful for array reading
    virtual bool hasMore();

    virtual bool serialise(); // let external operators find out they should serialise or deserialise
    // return true if no serialisation/deserialisation error occoured
    virtual bool isOK();
    virtual void setError(); // let external operators set the failed bit
    //virtual void addLogMsg(std::string msg); // mayb remove? (put log messages to error log einstead)
    virtual void addErrorMsg(std::string msg);
    virtual std::string getLog();
    virtual std::string getErrorLog();

    virtual bool isRawData();
    virtual std::string getRawData();
private:
    bool mSerialise;
    enum DataType{ TYPE_UNDEFINED, TYPE_ARRAY, TYPE_OBJECT, TYPE_RAW };
    // check if the current type is undefined
    // if not check if the new type matches the old type
    // if not set the error bit
    void setType(DataType type);
    DataType mDataType;

    json::Value mValue;

    json::Object mObject;
    // check if we are and object
    // check if this key exists
    bool checkObjectMember(std::string key);
    json::Array mArray;
    size_t mArrayNextRead;
    // check if we are an array
    // check if next read is valid
    // if not set error bit
    bool arrayBoundsOk();
    std::string mRawString;

    bool mIsOk;
    std::string mErrorLog;

    // try serialisation and set error bit on error
    bool checkDeserialisation();

    // check if value has correct type
    // if yes return the extracted value
    // if not then set the error bit
    void valueToBool(json::Value& value, bool& boolean);
    void valueToInt(json::Value& value, int& integer);
    void valueToDouble(json::Value& value, double& doubleVal);
    void valueToString(json::Value& value, std::string& str);

    void deleteCurrentChild();
    json::Value getJsonValue();
    JsonStream* mChild;
    std::string mChildKey;
};

} // namespace resource_api
