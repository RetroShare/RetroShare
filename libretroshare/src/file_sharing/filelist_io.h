#pragma once

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "util/rsmemory.h"

// This file implements load/save of various fields used for file lists and directory content.
// WARNING: the encoding is system-dependent, so this should *not* be used to exchange data between computers.

static const uint8_t DIRECTORY_STORAGE_VERSION           =  0x01 ;

static const uint8_t FILE_LIST_IO_TAG_FILE_SHA1_HASH     =  0x01 ;
static const uint8_t FILE_LIST_IO_TAG_FILE_NAME          =  0x02 ;
static const uint8_t FILE_LIST_IO_TAG_FILE_SIZE          =  0x03 ;
static const uint8_t FILE_LIST_IO_TAG_DIR_NAME           =  0x04 ;
static const uint8_t FILE_LIST_IO_TAG_MODIF_TS           =  0x05 ;
static const uint8_t FILE_LIST_IO_TAG_RECURS_MODIF_TS    =  0x06 ;
static const uint8_t FILE_LIST_IO_TAG_HASH_STORAGE_ENTRY =  0x07 ;
static const uint8_t FILE_LIST_IO_TAG_UPDATE_TS          =  0x08 ;
static const uint8_t FILE_LIST_IO_TAG_BINARY_DATA        =  0x09 ;
static const uint8_t FILE_LIST_IO_TAG_RAW_NUMBER         =  0x0a ;
static const uint8_t FILE_LIST_IO_TAG_ENTRY_INDEX        =  0x0b ;
static const uint8_t FILE_LIST_IO_TAG_REMOTE_FILE_ENTRY  =  0x0c ;

static const uint32_t SECTION_HEADER_MAX_SIZE            =  6 ;   // section tag (1 byte) + size (max = 5 bytes)

class FileListIO
{
public:
    template<typename T>
    static bool writeField(unsigned char *& buff,uint32_t& buff_size,uint32_t& offset,uint8_t section_tag,const T& val)
    {
        uint32_t s = serial_size(val) ;

       if(!checkSectionSize(buff,buff_size,offset,s))
          return false;

       if(!writeSectionHeader(buff,buff_size,offset,section_tag,s))
          return false;

       return serialise(buff,buff_size,offset,val) ;
    }

    template<typename T>
    static bool readField(const unsigned char *buff,uint32_t buff_size,uint32_t& offset,uint8_t check_section_tag,T& val)
    {
        uint32_t section_size ;

       if(!readSectionHeader(buff,buff_size,offset,check_section_tag,section_size))
          return false;

       return deserialise(buff,buff_size,offset,val);
    }

    static bool writeField(      unsigned char*&buff,uint32_t& buff_size,uint32_t& offset,uint8_t       section_tag,const unsigned char *  val,uint32_t  size) ;
    static bool readField (const unsigned char *buff,uint32_t  buff_size,uint32_t& offset,uint8_t check_section_tag,      unsigned char *& val,uint32_t& size) ;

    template<class T> static bool serialise(unsigned char *buff,uint32_t size,uint32_t& offset,const T& val) ;
    template<class T> static bool deserialise(const unsigned char *buff,uint32_t size,uint32_t& offset,T& val) ;
    template<class T> static uint32_t serial_size(const T& val) ;

    static bool saveEncryptedDataToFile(const std::string& fname,const unsigned char *data,uint32_t total_size);
    static bool loadEncryptedDataFromFile(const std::string& fname,unsigned char *& data,uint32_t& total_size);

private:
    static bool write125Size(unsigned char *data,uint32_t total_size,uint32_t& offset,uint32_t size) ;
    static bool read125Size (const unsigned char *data,uint32_t total_size,uint32_t& offset,uint32_t& size) ;

    static bool checkSectionSize(unsigned char *& buff,uint32_t& buff_size,uint32_t offset,uint32_t S)
    {
        if(offset + S + SECTION_HEADER_MAX_SIZE > buff_size)
        {
            buff = (unsigned char *)realloc(buff,offset + S + SECTION_HEADER_MAX_SIZE) ;
            buff_size = offset + S + SECTION_HEADER_MAX_SIZE;

            if(!buff)
               return false ;
        }
        return true ;
    }

    static bool writeSectionHeader(unsigned char *& buff,uint32_t& buff_size,uint32_t& offset,uint8_t section_tag,uint32_t S)
    {
        buff[offset++] = section_tag ;
        if(!write125Size(buff,buff_size,offset,S)) return false ;

        return true;
    }

    static bool readSectionHeader(const unsigned char *& buff,uint32_t buff_size,uint32_t& offset,uint8_t check_section_tag,uint32_t& S)
    {
        if(offset + 1 > buff_size)
            return false ;

        uint8_t section_tag = buff[offset++] ;

        if(section_tag != check_section_tag)
            return false;

        return read125Size(buff,buff_size,offset,S) ;
    }
};
