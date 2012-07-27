
#include <memory.h>

#include "retrodb.h"
#include "utest.h"

bool operator==(const ContentValue& lCV, ContentValue& rCV){

    std::map<std::string, uint8_t> leftKtMap, rightKtMap;
    lCV.getKeyTypeMap(leftKtMap);
    rCV.getKeyTypeMap(rightKtMap);

    return (leftKtMap == rightKtMap);
}


/*!
 * Test content value ability to take a set of data and ability to get it out
 * For all data types
 */
void testEnterAndRetrieve();

/*!
 * Check to see if copy constructor intialises
 * 'this' correctly
 */
void testCopyConstructor();

/*!
 * check that key type map returned is consistent
 * with data contained in ContentValue
 */
void testGetKeyTypeMap();

/*!
 * Test the clearing functionality
 */
void testClear();

/*!
 * check errors are meaningful
 */
void testErrors();

/*!
 * enter the same key twice and ensure previous data gets overwritten
 *
 */
void testSameKey();


INITTEST();

int main(){

    testEnterAndRetrieve();
    testCopyConstructor();
    testClear();
    testSameKey();
    testGetKeyTypeMap();
    testErrors();

    FINALREPORT("TEST_CONTENT_VALUE");
}


void testSameKey(){


    ContentValue cv;

    // test data

    std::string src = "adalkjalfalfalfkal";
    const char* data = src.data();
    uint32_t data_len = src.length();
    std::string data_key = "private_key";
    cv.put(data_key, data_len, data);

    std::string other_src = "daldko5202402224";
    data_len = other_src.length();
    data = other_src.data();

    cv.put(data_key, data_len, data);

    uint32_t val_len;
    char* val;
    cv.getAsData(data_key, val_len, val);
    std::string str_val(val, val_len);


    CHECK(str_val == other_src);

    // now check string

    std::string key = "peer";
    cv.put(key, std::string("sexy_girl"));
    cv.put(key, std::string("manly man"));

    std::string val_str;
    cv.getAsString(key, val_str);

    CHECK(val_str == "manly man");

    // and double

    cv.put(key, 4.);
    cv.put(key, 32.);
    double val_d;
    cv.getAsDouble(key, val_d);
    CHECK(val_d == 32.);

    // and int64
    int64_t large(20420492040123);
    cv.put(key, large);
    cv.put(key, large+34);
    int64_t val_i64;
    cv.getAsInt64(key, val_i64);

    CHECK(val_i64 == large+34);

    // and bool

    cv.put(key, false);
    cv.put(key, true);
    bool bool_val = false;
    cv.getAsBool(key, bool_val);

    CHECK(bool_val == true);

    // and int32

    int32_t medium = 20432123;
    cv.put(key, medium);
    cv.put(key, medium+34);
    int32_t val_i32;
    cv.getAsInt32(key, val_i32);

    CHECK(val_i32 == medium+34);

    REPORT("testSameKey()");
}


void testErrors(){


    ContentValue cv;
    int32_t val_32;
    int64_t val_64;
    char* data;
    uint32_t data_len;
    std::string val_str;
    bool val_bool;
    double val_d;
    CHECK(!cv.getAsInt32("dda", val_32));
    CHECK(!cv.getAsInt64("dda", val_64));
    CHECK(!cv.getAsData("ds", data_len, data));
    CHECK(!cv.getAsString("d3536356336356356s", val_str));
    CHECK(!cv.getAsBool("d424s", val_bool));
    CHECK(!cv.getAsDouble("daads", val_d));

    REPORT("testErrors()");
}

void testEnterAndRetrieve(){

    // INT 32
    int32_t value32 = 1, retval32;
    std::string key = "one";

    ContentValue cv;
    cv.put(key, value32);

    cv.getAsInt32(key, retval32);
    CHECK(value32 == retval32);

    // INT 64
    int64_t value64 = 423425242, retval64;
    key = "int64";
    cv.put(key, value64);
    cv.getAsInt64(key, retval64);
    CHECK(value64 == retval64);


    // Double
    double dvalue = 3.5, retvaldbl;
    key = "double";
    cv.put(key, dvalue);
    cv.getAsDouble(key, retvaldbl);
    CHECK(dvalue == retvaldbl );

    // BLOB
    uint32_t data_len = 45, get_len = 0;

    char* src = new char[data_len];
    char* get_src = NULL;
    memset(src, '$', data_len);
    key = "data";
    cv.put(key, data_len, src);

     cv.getAsData(key, get_len, get_src);

    bool fine = true;

    CHECK(get_len = data_len);

    if(data_len == get_len){

        for(int i=0; i < data_len; i++){

            if(src[i] != get_src[i])
                fine &= false;

        }
    }

    delete[] src;
    CHECK(fine);

    // STRING
    std::string strVal = "whoo", getVal("");
    key = "string";
    cv.put(key, strVal);
    cv.getAsString(key, getVal);

    CHECK(getVal == strVal);


    // BOOL
    bool mefalse = false, retvalBool;
    key = "bool";
    cv.put(key, mefalse);
    cv.getAsBool(key, retvalBool);
    CHECK(mefalse == retvalBool);

    cv.clear();

    REPORT("testEnterAndRetrieve()");
}

void testGetKeyTypeMap(){

    ContentValue cv;
    std::string key1="key1", key2="key2", key3="key3", key4="key4";
    std::string key1_val;
    int32_t key2_val = 42;
    double key3_val = 23;
    int64_t key4_val = 42052094224;

    cv.put(key1, key1_val);
    cv.put(key2, key2_val);
    cv.put(key3, key3_val);
    cv.put(key4, key4_val);

    std::map<std::string, uint8_t> kvMap;

    cv.getKeyTypeMap(kvMap);

    CHECK(kvMap.size() == 4);
    CHECK(kvMap[key1] == ContentValue::STRING_TYPE);
    CHECK(kvMap[key2] == ContentValue::INT32_TYPE);
    CHECK(kvMap[key3] == ContentValue::DOUBLE_TYPE);
    CHECK(kvMap[key4] == ContentValue::INT64_TYPE);

    REPORT("testGetKeyTypeMap()");
}

void testCopyConstructor(){

    ContentValue cv1;

    // INT 32
    int value32 = 1;
    std::string key = "one";

    cv1.put(key, value32);

    // INT 64
    int64_t value64 = 423425242;
    key = "int64";
    cv1.put(key, value64);

    // Double
    double dvalue = 3.5;
    key = "double";
    cv1.put(key, dvalue);

    // BLOB
    uint32_t data_len = 45;
    char* src = new char[data_len];

    memset(src, '$', data_len);
    key = "data";
    cv1.put(key, data_len, src);


    delete[] src;

    // STRING
    std::string strVal = "whoo";
    key = "string";
    cv1.put(key, strVal);

    // BOOL
    bool mefalse = false;
    key = "bool";
    cv1.put(key, mefalse);

    ContentValue cv2(cv1);

    CHECK(cv1 == cv2);

    cv1.clear();
    cv2.clear();

    REPORT("testCopyConstructor()");
}


void testClear(){

    ContentValue cv1;

    // INT 32
    int value32 = 1;
    std::string key = "one";

    cv1.put(key, value32);

    // INT 64
    int64_t value64 = 423425242;
    key = "int64";
    cv1.put(key, value64);

    // Double
    double dvalue = 3.5;
    key = "double";
    cv1.put(key, dvalue);

    // BLOB
    uint32_t data_len = 45;
    char* src = new char[data_len];

    memset(src, '$', data_len);
    key = "data";
    cv1.put(key, data_len, src);


    delete[] src;

    // STRING
    std::string strVal = "whoo";
    key = "string";
    cv1.put(key, strVal);

    // BOOL
    bool mefalse = false;
    key = "bool";
    cv1.put(key, mefalse);

    std::map<std::string, uint8_t> ktMap;

    cv1.getKeyTypeMap(ktMap);
    CHECK(ktMap.size() > 0);

    cv1.clear();

    cv1.getKeyTypeMap(ktMap);
    CHECK(ktMap.size() == 0);

    REPORT("testClear()");

}
