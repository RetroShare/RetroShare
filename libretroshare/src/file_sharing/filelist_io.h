/*******************************************************************************
 * libretroshare/src/file_sharing: filelist_io.h                               *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2018 by Mr.Alice <mralice@users.sourceforge.net>                  *
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
 ******************************************************************************/
#pragma once

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "util/rsmemory.h"

// This file implements load/save of various fields used for file lists and directory content.
// WARNING: the encoding is system-dependent, so this should *not* be used to exchange data between computers.

static const uint32_t FILE_LIST_IO_LOCAL_DIRECTORY_STORAGE_VERSION_0001 =  0x00000001 ;
static const uint32_t FILE_LIST_IO_LOCAL_DIRECTORY_TREE_VERSION_0001    =  0x00010001 ;

static const uint8_t FILE_LIST_IO_TAG_UNKNOWN                   =  0x00 ;
static const uint8_t FILE_LIST_IO_TAG_LOCAL_DIRECTORY_VERSION   =  0x01 ;

static const uint8_t FILE_LIST_IO_TAG_HASH_STORAGE_ENTRY        =  0x10 ;
static const uint8_t FILE_LIST_IO_TAG_LOCAL_FILE_ENTRY          =  0x11 ;
static const uint8_t FILE_LIST_IO_TAG_LOCAL_DIR_ENTRY           =  0x12 ;
static const uint8_t FILE_LIST_IO_TAG_REMOTE_FILE_ENTRY         =  0x13 ;

static const uint8_t FILE_LIST_IO_TAG_FILE_SHA1_HASH            =  0x20 ;
static const uint8_t FILE_LIST_IO_TAG_FILE_NAME                 =  0x21 ;
static const uint8_t FILE_LIST_IO_TAG_FILE_SIZE                 =  0x22 ;

static const uint8_t FILE_LIST_IO_TAG_MODIF_TS                  =  0x30 ;
static const uint8_t FILE_LIST_IO_TAG_RECURS_MODIF_TS           =  0x31 ;
static const uint8_t FILE_LIST_IO_TAG_UPDATE_TS                 =  0x32 ;

static const uint8_t FILE_LIST_IO_TAG_ENTRY_INDEX               =  0x40 ;
static const uint8_t FILE_LIST_IO_TAG_PARENT_INDEX              =  0x41 ;

static const uint8_t FILE_LIST_IO_TAG_DIR_HASH                  =  0x50 ;
static const uint8_t FILE_LIST_IO_TAG_DIR_NAME                  =  0x51 ;

static const uint8_t FILE_LIST_IO_TAG_ROW                       =  0x60 ;
static const uint8_t FILE_LIST_IO_TAG_BINARY_DATA               =  0x61 ;
static const uint8_t FILE_LIST_IO_TAG_RAW_NUMBER                =  0x62 ;

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

	class read_error
	{
	public:
		read_error(unsigned char *sec,uint32_t size,uint32_t offset,uint8_t expected_tag);
		read_error(const std::string& s) : err_string(s) {}

		const std::string& what() const { return err_string ; }
	private:
		std::string err_string ;
	};


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
        // This tests avoids an infinite loop when growing new size

        if(offset + S + SECTION_HEADER_MAX_SIZE > 0x8fffffff)
            return false ;

        if(offset + S + SECTION_HEADER_MAX_SIZE > buff_size)
        {
            uint32_t new_size = (buff_size == 0)?512:buff_size ;

            while(new_size < offset + S + SECTION_HEADER_MAX_SIZE)
                new_size <<= 1 ;

            buff = (unsigned char *)realloc(buff,new_size) ;

            if(!buff)
            {
                buff_size = 0 ;
               return false ;
            }
            buff_size = new_size ;
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

        uint8_t section_tag = buff[offset] ;			// we do the offset++ after, only if the header can be read. Doing so, we can make multiple read attempts.

        if(section_tag != check_section_tag)
            return false;

        offset++ ;

        return read125Size(buff,buff_size,offset,S) ;
    }
};
