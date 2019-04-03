/*******************************************************************************
 * libretroshare/src/util: contentvalue.cc                                     *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2012 Christopher Evi-Parker <retroshare@lunamutt.com>             *
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
#include <iostream>
#include <memory.h>

#include "contentvalue.h"




const uint8_t ContentValue::BOOL_TYPE  = 1;
const uint8_t ContentValue::DATA_TYPE = 2;
const uint8_t ContentValue::STRING_TYPE = 3;
const uint8_t ContentValue::DOUBLE_TYPE = 4;
const uint8_t ContentValue::INT32_TYPE = 5;
const uint8_t ContentValue::INT64_TYPE = 6;


/**************** content value implementation ******************/

typedef std::pair<std::string, uint8_t> KeyTypePair;

ContentValue::ContentValue(){

}

ContentValue::~ContentValue(){
    // release resources held in data
    clearData();
}

ContentValue::ContentValue(const ContentValue &from){

    std::map<std::string, uint8_t> keyTypeMap;
    from.getKeyTypeMap(keyTypeMap);
    std::map<std::string, uint8_t>::const_iterator cit =
            keyTypeMap.begin();

    uint8_t type = 0;
    std::string currKey;
    std::string val = "";
    char *src = NULL;
    uint32_t data_len = 0;

    for(; cit != keyTypeMap.end(); ++cit){

        type = cit->second;
        currKey = cit->first;

        switch(type){

        case INT32_TYPE:
            {
                int32_t value = 0;
                if (from.getAsInt32(currKey, value))
                    put(currKey, value);
                break;
            }
        case INT64_TYPE:
            {
                int64_t value = 0;
                if (from.getAsInt64(currKey, value))
                    put(currKey, value);
                break;
            }
        case STRING_TYPE:
            {
                if (from.getAsString(currKey, val))
                    put(currKey, val);
                break;
            }
        case BOOL_TYPE:
            {
                bool value = false;
                if (from.getAsBool(currKey, value))
                    put(currKey, value);
                break;
            }
        case DATA_TYPE:
            {
                if (from.getAsData(currKey, data_len, src))
                    put(currKey, data_len, src);
                break;
            }
        case DOUBLE_TYPE:
            {
                double value = 0;
                if (from.getAsDouble(currKey, value))
                    put(currKey, value);
                break;
            }
        default:
            std::cerr << "ContentValue::ContentValue(ContentValue &from):"
                    << "Error! Unrecognised data type!" << std::endl;
            break;
        }
    }
}

void ContentValue::put(const std::string &key, bool value){

    if(mKvSet.find(key) != mKvSet.end())
        removeKeyValue(key);

    mKvSet.insert(KeyTypePair(key, BOOL_TYPE));
    mKvBool.insert(std::pair<std::string, bool>(key, value));
}

void ContentValue::put(const std::string &key, const std::string &value){

    if(mKvSet.find(key) != mKvSet.end())
        removeKeyValue(key);

    mKvSet.insert(KeyTypePair(key, STRING_TYPE));
    mKvString.insert(std::pair<std::string, std::string>(key, value));
}

void ContentValue::put(const std::string &key, double value){

    if(mKvSet.find(key) != mKvSet.end())
        removeKeyValue(key);

    mKvSet.insert(KeyTypePair(key,DOUBLE_TYPE));
    mKvDouble.insert(std::pair<std::string, double>(key, value));
}

void ContentValue::put(const std::string &key, int32_t value){

    if(mKvSet.find(key) != mKvSet.end())
        removeKeyValue(key);

    mKvSet.insert(KeyTypePair(key, INT32_TYPE));
    mKvInt32.insert(std::pair<std::string, int32_t>(key, value));
}

void ContentValue::put(const std::string &key, int64_t value){

    if(mKvSet.find(key) != mKvSet.end())
        removeKeyValue(key);

    mKvSet.insert(KeyTypePair(key, INT64_TYPE));
    mKvInt64.insert(std::pair<std::string, int64_t>(key, value));
}

void ContentValue::put(const std::string &key, uint32_t len, const char* value){


    // release memory from old key value if key
    // exists
    if(mKvSet.find(key) != mKvSet.end()) {
        removeKeyValue(key);
    }

    mKvSet.insert(KeyTypePair(key, DATA_TYPE));
    char* dest = NULL;

    // len is zero then just put a NULL entry
    if(len != 0){
        dest  = new char[len];
        memcpy(dest, value, len);
    }

    mKvData.insert(std::pair<std::string, std::pair<uint32_t, char*> >
                   (key, std::pair<uint32_t, char*>(len, dest)));
    //delete[] dest; //Deleted by clearData()
    // cppcheck-suppress memleak
}

bool ContentValue::getAsBool(const std::string &key, bool& value) const{

    std::map<std::string, bool>::const_iterator it;
    if((it = mKvBool.find(key)) == mKvBool.end())
        return false;

  value = it->second;
  return true;
}

bool ContentValue::getAsInt32(const std::string &key, int32_t& value) const{

    std::map<std::string, int32_t>::const_iterator it;
    if((it = mKvInt32.find(key)) == mKvInt32.end())
        return false;

    value = it->second;
    return true;
}

bool ContentValue::getAsInt64(const std::string &key, int64_t& value) const{

    std::map<std::string, int64_t>::const_iterator it;
    if((it = mKvInt64.find(key)) == mKvInt64.end())
        return false;

    value = it->second;
    return true;
}

bool ContentValue::getAsString(const std::string &key, std::string &value) const{

    std::map<std::string, std::string>::const_iterator it;
    if((it = mKvString.find(key)) == mKvString.end())
        return false;

    value = it->second;
    return true;
}

bool ContentValue::getAsData(const std::string& key, uint32_t &len, char*& value) const{

    std::map<std::string, std::pair<uint32_t, char*> >::const_iterator it;
    if((it = mKvData.find(key)) == mKvData.end())
        return false;

    const std::pair<uint32_t, char*> &kvRef = it->second;

    len = kvRef.first;
    value = kvRef.second;
    return true;
}

bool ContentValue::getAsDouble(const std::string &key, double& value) const{

    std::map<std::string, double>::const_iterator it;
    if((it = mKvDouble.find(key)) == mKvDouble.end())
        return false;

    value = it->second;
    return true;
}

bool ContentValue::removeKeyValue(const std::string &key){

    std::map<std::string, uint8_t>::iterator mit;

    if((mit = mKvSet.find(key)) == mKvSet.end())
        return false;

    if(mit->second == BOOL_TYPE)
        mKvBool.erase(key);

    if(mit->second == INT64_TYPE)
        mKvInt64.erase(key);

    if(mit->second == DATA_TYPE){
        delete[] (mKvData[key].second);
        mKvData.erase(key);
    }

    if(mit->second == DOUBLE_TYPE)
        mKvDouble.erase(key);

    if(mit->second == STRING_TYPE)
        mKvString.erase(key);

    if(mit->second == INT32_TYPE)
        mKvInt32.erase(key);


    mKvSet.erase(key);
    return true;
}


void ContentValue::getKeyTypeMap(std::map<std::string, uint8_t> &keySet) const {
    keySet = mKvSet;
}

void ContentValue::clear(){
    mKvSet.clear();
    mKvBool.clear();
    mKvDouble.clear();
    mKvString.clear();
    mKvInt32.clear();
    mKvInt64.clear();
    clearData();
}

bool ContentValue::empty() const{
    return mKvSet.empty();
}

void ContentValue::clearData(){

    std::map<std::string, std::pair<uint32_t, char*> >::iterator
            mit = mKvData.begin();

    for(; mit != mKvData.end(); ++mit){

        if(mit->second.first != 0)
            delete[] (mit->second.second);
    }

    mKvData.clear();
}



