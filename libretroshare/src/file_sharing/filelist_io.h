#pragma once

#include <stdint.h>

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

class FileListIO
{
public:
    template<typename T>
    static bool writeField(unsigned char *& buff,uint32_t& buff_size,uint32_t& offset,uint8_t section_tag,const T& val)
    {
       if(!checkSectionSize(buff,buff_size,offset,sizeof(T)))
          return false;

       if(!writeSectionHeader(buff,buff_size,offset,section_tag,sizeof(T)))
          return false;

       memcpy(&buff[offset],reinterpret_cast<const uint8_t*>(&val),sizeof(T)) ;
       offset += sizeof(T) ;

       return true;
    }

private:
    static bool checkSectionSize(unsigned char *& buff,uint32_t& buff_size,uint32_t offset,uint32_t S)
    {
        if(offset + S > buff_size)
        {
            buff = (unsigned char *)realloc(buff,offset + S) ;
            buff_size = offset + S ;

            if(!buff)
               return false ;
        }
        return true ;
    }

    static bool writeSectionHeader(unsigned char *& buff,uint32_t& buff_size,uint32_t offset,uint8_t section_tag,uint32_t S)
    {
        buff[offset++] = section_tag ;
        if(!write125Size(buff,offset,buff_size,S)) return false ;

        return true;
    }
};
