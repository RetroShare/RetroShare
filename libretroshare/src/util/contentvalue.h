/*******************************************************************************
 * libretroshare/src/util: contentvalue.h                                      *
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
#ifndef CONTENTVALUE_H
#define CONTENTVALUE_H

#include <inttypes.h>
#include <string>
#include <map>

/*!
 * @brief Convenience container for making additions to databases
 * This class provides a means of holding column values to insert into a database
 */
class ContentValue {

public:

    static const uint8_t INT32_TYPE;
    static const uint8_t INT64_TYPE;
    static const uint8_t DOUBLE_TYPE;
    static const uint8_t STRING_TYPE;
    static const uint8_t DATA_TYPE;
    static const uint8_t BOOL_TYPE;

    ContentValue();

    /*!
     * copy constructor that copys the key value set from another \n
     * ContentValue object to this one
     * makes a deep copy of raw data
     * @param from ContentValue instance to copy key value set from
     */
    ContentValue(const ContentValue& from);//const damit die äußere klasse einen konstruktor com compielr bekommt

    /*!
     *
     *
     *
     */
    ~ContentValue();

    /*!
     * Adds a value to the set
     * @param key  the name of the value to put
     * @param value  the data for the value to put
     * @warning cast string literals explicitly as string, observed string literal \n
     *          being casted to bool instead e.g. string("hello") rather than "hello"
     */
    void put(const std::string& key, const std::string& value);

    /*!
     * Adds a value to the set
     * @param key  the name of the value to put
     * @param value  the data for the value to put
     */
    void put(const std::string& key, bool value);

    /*!
     * Adds a value to the set
     * @param key  the name of the value to put
     * @param value  the data for the value to put
     */
    void put(const std::string& key, int64_t value);

    /*!
     * Adds a value to the set
     * @param key  the name of the value to put
     * @param value  the data for the value to put
     */
    void put(const std::string& key, int32_t value);

    /*!
     * Adds a value to the set
     * @param key  the name of the value to put
     * @param value  the data for the value to put
     */
    void put(const std::string& key, double value);

    /*!
     * Adds a value to the set
     * Takes a private copy of data
     * @param key  the name of the value to put
     * @param value  the data for the value to put
     */
    void put(const std::string& key, uint32_t len, const char* value);


    /*!
     * get value as 32 bit signed integer
     * @param key the value to get
     */
    bool getAsInt32(const std::string& key, int32_t& value) const;

    /*!
     * get value as 64 bit signed integer
     * @param key the value to get
     */
    bool getAsInt64(const std::string& key, int64_t& value) const;

    /*!
     * get value as bool
     * @param key the value to get
     */
    bool getAsBool(const std::string& key, bool& value) const;

    /*!
     * get as value double
     * @param key the value to get
     */
    bool getAsDouble(const std::string& key, double& value) const;

    /*!
     * get as value as string
     * @param key the value to get
     * @param value the data retrieved
     */
    bool getAsString(const std::string& key, std::string& value) const;

    /*!
     * get as value as raw data
     * @warning Deep copy of data reference should be made, if this instance ContentValue \n
     *          is destroyed pointer returned (value) is pointing to invalid memory
     * @param key the value to get
     */
    bool getAsData(const std::string&, uint32_t& len, char*& value) const;

    /*!
     * @param keySet the is set with key to type pairs contained in the ContentValue instance
     */
    void getKeyTypeMap(std::map<std::string, uint8_t>& keySet) const;

    /*!
     * @param key the key of the key value pair to remove
     * @return true if key was found and removed, false otherwise
     */
    bool removeKeyValue(const std::string& key);

    /*!
     * clears this data structure of all its key value pairs held
     */
    void clear();

    /*!
     * checks if internal kv map is empty
     */
    bool empty() const;

private:

    /*!
     * release memory resource associated with mKvData
     */
    void clearData();

private:

    std::map<std::string, int32_t> mKvInt32;
    std::map<std::string, int64_t> mKvInt64;
    std::map<std::string, double> mKvDouble;
    std::map<std::string, std::string> mKvString;
    std::map<std::string, std::pair<uint32_t, char*> > mKvData;
    std::map<std::string, bool> mKvBool;

    std::map<std::string, uint8_t> mKvSet;

};

#endif // CONTENTVALUE_H
