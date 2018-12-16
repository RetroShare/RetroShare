/*******************************************************************************
 * libresapi/api/ApiTypes.h                                                    *
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

#include <string>
#include <vector>
#include <stack>
#include <stdint.h>
#include <ostream>

#include "util/rsdeprecate.h"

namespace resource_api
{
// things to clean up:
// - temporary variables when serialising rs-ids
// - always ensure proper return values
// - add help functions
// - remove unused functions or implement them
// - add operators or functions for std::set, std::list, std::vector, std::map


// idea:
// make a second parameter like
// ValueReference(this->member, OPTIONAL); // optional signals that it is not an error if this member is missing
// make a third parameter with a type hint: time, length, kilobytes

// to make arrays
template<class T>
class ValueReference
{
public:
    ValueReference(T& value): value(value){}
    T& value;
};

template<class T>
ValueReference<T> makeValueReference(T& value);

template<class T>
class Value
{
public:
    Value(T value): value(value){}
    operator ValueReference<T>(){ return ValueReference<T>(value);}
    T value;
};

template<class T>
Value<T> makeValue(T value);

// to make objects
template<class T>
class KeyValueReference
{
public:
    KeyValueReference(std::string key, T& value): key(key), value(value){}
    //KeyValueReference(const char* key, T& value): key(key), value(value){}
    std::string key;
    T& value;
};

template<class T>
KeyValueReference<T> makeKeyValueReference(std::string key, T& value);

// for serialisation
// copies the supplied value
// automatically converts itself to a KeyValueReference
template<class T>
class KeyValue
{
public:
    KeyValue(std::string key, T value): key(key), value(value){}

    operator KeyValueReference<T>(){ return KeyValueReference<T>(key, value);}

    std::string key;
    T value;
};

template<class T>
KeyValue<T> makeKeyValue(std::string key, T value);

// interface for streams
class StreamBase
{
public:
    // the stream provides operators for basic data types
    // everything else should be broken down by others
    // the same stream can either become an object or an array stream, or a binary data object
    // a binary data object is just raw binary data without any decoration
    // binary data is good to pass things like images and small files
    // this depends on how this stream is used
    // but once the stream is used as array, then only array operations are allowed
    // same with an stream used as object

    // idea: can have filter streams which forward the calls to another stream
    //       to make debug protocols of other steam implementations
    // idea: make a stream shich creates a hash from the input to detect changes in the data

    // make an array
    virtual StreamBase& operator<<(ValueReference<bool> value) = 0;
    virtual StreamBase& operator<<(ValueReference<int> value) = 0;
    virtual StreamBase& operator<<(ValueReference<double> value) = 0;
    virtual StreamBase& operator<<(ValueReference<std::string> value) = 0;
    // usefull if the new array member should be an array or object
    // the reference should be at least valid until another method of this class gets called
    virtual StreamBase& getStreamToMember() = 0;

    // make an object
    virtual StreamBase& operator<<(KeyValueReference<bool> keyValue) = 0;
    virtual StreamBase& operator<<(KeyValueReference<int> keyValue) = 0;
    virtual StreamBase& operator<<(KeyValueReference<double> keyValue) = 0;
    virtual StreamBase& operator<<(KeyValueReference<std::string> keyValue) = 0;
    // usefull if the new object member should be an array or object
    // the reference should be at least valid until another method of this class gets called
    virtual StreamBase& getStreamToMember(std::string name) = 0;

    // make a binay data object (not a real object, just binary data)
    // idea: can use vector.swap() to allow passing larger data items without copying
    virtual StreamBase& operator<<(std::vector<uint8_t>& data) = 0;

    // return true if there are more members in this object/array
    // useful for array reading
    virtual bool hasMore() = 0;

    virtual bool serialise() = 0; // let external operators find out they should serialise or deserialise
    // return true if no serialisation/deserialisation error occoured
    virtual bool isOK() = 0;
    virtual void setError() = 0; // let external operators set the failed bit
    //virtual void addLogMsg(std::string msg) = 0;
    virtual void addErrorMsg(std::string msg) = 0;
    virtual std::string getLog() = 0;
    virtual std::string getErrorLog() = 0;

    virtual bool isRawData() = 0;
    virtual std::string getRawData() = 0;// HACK, remove this
};

// todo:
// define clear rules how a response to a request should look like
// the clients should be able to know when something failed
// then it is desired to have a minimum of debug output to track the errors down
// currently no check for successful serialisation/deserialisation is performed
//
// response metadata:
// - etag, when will this result expire
// - maybe a hint how often this etag should be checked for updates
//
// outcome of a request:
// - full ok
// - partial ok
// - resource not found, invalid address or resource not available
// - not ok, internal error
// - wrong usage, parameters or POST data is wrong, like deserialisation error
// is is hard to find the cause of the error
// maybe include a comment with additional info
//
// want to include a mime type of the resulting data?
// because some data is json, othe plain text, other unknown binary stuff

// live-stream resources
// some resources like typing notifications are only valid for a short time

// resource types:
// - list, either with objects or adresses of objects.
//   lists need a navigation mechanism like get objects before and get objects after
// - object
// - stream
// - binary data, for example files

// TODO: record a timestamp for each token, to allow garbage collection of very old tokens
class StateToken{
public:
    StateToken(): value(0){}
    StateToken(uint32_t value): value(value){}
    std::string toString();

    uint32_t getValue() const {return value;}
    bool isNull() const {return value == 0;}
private:
    uint32_t value; // 0 is reserved for invalid token
};

class Request
{
public:
	Request(StreamBase& stream) : mStream(stream), mMethod(GET){}

	RS_DEPRECATED bool isGet(){ return mMethod == GET;}
	RS_DEPRECATED bool isPut(){ return mMethod == PUT;}
	RS_DEPRECATED bool isDelete(){ return mMethod == DELETE_AA;}
	RS_DEPRECATED bool isExec(){ return mMethod == EXEC;}

	/**
	 * Path is the adress to the resource if the path has multiple parts which
	 * get handled by different handlers, then each handler should pop the top
	 * element
	 */
	std::stack<std::string> mPath;
	std::string mFullPath;
	bool setPath(const std::string &reqPath)
	{
		std::string str;
		std::string::const_reverse_iterator sit;
		for( sit = reqPath.rbegin(); sit != reqPath.rend(); ++sit )
		{
			// add to front because we are traveling in reverse order
			if((*sit) != '/') str = *sit + str;
			else if(!str.empty())
			{
				mPath.push(str);
				str.clear();
			}
		}
		if(!str.empty()) mPath.push(str);
		mFullPath = reqPath;

		return true;
	}

	/// Contains data for new resources
	StreamBase& mStream;

	/**
	 * @deprecated
	 * Method and derivated stuff usage is deprecated as it make implementation
	 * more complex and less readable without advantage
	 */
	enum Method { GET, PUT, DELETE_AA, EXEC};
	RS_DEPRECATED Method mMethod;
};

// new notes on responses
// later we want to send multiple requests over the same link
// and we want to be able to send the responses in a different order than the requests
// for this we need a unique token in every request which gets returned in the response

// response:
// message token
// status (ok, warning, fail)
// data (different for different resources)
// debugstring (a human readable error message in case something went wrong)

class Response
{
public:
    Response(StreamBase& stream, std::ostream& debug): mReturnCode(NOT_SET), mDataStream(stream), mDebug(debug){}

    // WARNING means: a valid result is available, but an error occoured
    // FAIL means: the result is not valid
    enum ReturnCode{ NOT_SET, OK, WARNING, FAIL};
    ReturnCode mReturnCode;

    StateToken mStateToken;

	//Just for GUI benefit
	std::string mCallbackName;

    // the result
    StreamBase& mDataStream;

    // humand readable string for debug messages/logging
    std::ostream& mDebug;

    inline void setOk(){mReturnCode = OK;}
    inline void setWarning(std::string msg = ""){
        mReturnCode = WARNING;
        if(msg != "")
            mDebug << msg << std::endl;}
    inline void setFail(std::string msg = ""){
        mReturnCode = FAIL;
        if(msg != "")
            mDebug << msg << std::endl;
    }
};

// if a response can not be handled immediately,
// then the handler should return a ResponseTask object
// the api server will then call the doWork() method periodically
class ResponseTask
{
public:
    virtual ~ResponseTask(){}
    // return true if function should get called again
    // return false when finished
    virtual bool doWork(Request& req, Response& resp) = 0;
};

// implementations

template<class T>
ValueReference<T> makeValueReference(T& value)
{
    return ValueReference<T>(value);
}

template<class T>
Value<T> makeValue(T value)
{
    return Value<T>(value);
}

template<class T>
KeyValueReference<T> makeKeyValueReference(std::string key, T& value)
{
    return KeyValueReference<T>(key, value);
}

template<class T>
KeyValue<T> makeKeyValue(std::string key, T value)
{
    return KeyValue<T>(key, value);
}

} // namespace resource_api
