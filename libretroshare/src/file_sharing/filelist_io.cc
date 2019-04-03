/*******************************************************************************
 * libretroshare/src/file_sharing: filelist_io.cc                              *
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
#include <sstream>
#include "retroshare/rsids.h"
#include "pqi/authssl.h"
#include "util/rsdir.h"
#include "util/rsprint.h"
#include "serialiser/rsbaseserial.h"
#include "filelist_io.h"

FileListIO::read_error::read_error(unsigned char *sec,uint32_t size,uint32_t offset,uint8_t expected_tag)
{
	std::ostringstream s ;
	s << "At offset " << offset << "/" << size << ": expected section tag " << std::hex << (int)expected_tag << std::dec << " but got " << RsUtil::BinToHex(&sec[offset],std::min((int)size-(int)offset, 15)) << "..." << std::endl;
	err_string = s.str();
}

template<> bool FileListIO::serialise(unsigned char *buff,uint32_t size,uint32_t& offset,const uint32_t     & val)  { return setRawUInt32(buff,size,&offset,val) ; }
template<> bool FileListIO::serialise(unsigned char *buff,uint32_t size,uint32_t& offset,const uint64_t     & val)  { return setRawUInt64(buff,size,&offset,val) ; }
template<> bool FileListIO::serialise(unsigned char *buff,uint32_t size,uint32_t& offset,const std::string  & val)  { return setRawString(buff,size,&offset,val) ; }
template<> bool FileListIO::serialise(unsigned char *buff,uint32_t size,uint32_t& offset,const Sha1CheckSum & val)  { return val.serialise(buff,size,offset) ; }

template<> bool FileListIO::deserialise(const unsigned char *buff,uint32_t size,uint32_t& offset,uint32_t     & val)  { return getRawUInt32(const_cast<uint8_t*>(buff),size,&offset,&val) ; }
template<> bool FileListIO::deserialise(const unsigned char *buff,uint32_t size,uint32_t& offset,uint64_t     & val)  { return getRawUInt64(const_cast<uint8_t*>(buff),size,&offset,&val) ; }
template<> bool FileListIO::deserialise(const unsigned char *buff,uint32_t size,uint32_t& offset,std::string  & val)  { return getRawString(const_cast<uint8_t*>(buff),size,&offset,val) ; }
template<> bool FileListIO::deserialise(const unsigned char *buff,uint32_t size,uint32_t& offset,Sha1CheckSum & val)  { return val.deserialise(const_cast<uint8_t*>(buff),size,offset) ; }

template<> uint32_t FileListIO::serial_size(const uint32_t     &    )  { return 4 ; }
template<> uint32_t FileListIO::serial_size(const uint64_t       &    )  { return 8 ; }
template<> uint32_t FileListIO::serial_size(const std::string  & val)  { return getRawStringSize(val) ; }
template<> uint32_t FileListIO::serial_size(const Sha1CheckSum &    )  { return Sha1CheckSum::serial_size(); }

bool FileListIO::writeField(      unsigned char*&buff,uint32_t& buff_size,uint32_t& offset,uint8_t       section_tag,const unsigned char *  val,uint32_t  size)
{
    if(!checkSectionSize(buff,buff_size,offset,size))
        return false;

    if(!writeSectionHeader(buff,buff_size,offset,section_tag,size))
        return false;

    memcpy(&buff[offset],val,size) ;
    offset += size ;

    return true;
}

bool FileListIO::readField (const unsigned char *buff,uint32_t  buff_size,uint32_t& offset,uint8_t check_section_tag, unsigned char *& val,uint32_t& size)
{
    uint32_t local_size ;

    if(!readSectionHeader(buff,buff_size,offset,check_section_tag,local_size))
        return false;

    if(!checkSectionSize(val,size,0,local_size))	// allocate val if needed to handle local_size bytes.
        return false;

    memcpy(val,&buff[offset],local_size);
    offset += local_size ;

    return true ;
}

bool FileListIO::write125Size(unsigned char *data,uint32_t data_size,uint32_t& offset,uint32_t S)
{
    if(S < 192)
    {
        if(offset+1 > data_size)
            return false;

        data[offset++] = (uint8_t)S ;
        return true;
    }
    else if(S < 8384)
    {
        if(offset+2 > data_size)
            return false;

        data[offset+1] =  (S - 192) & 0xff ;
        data[offset  ] = ((S - 192 - data[offset+1]) >> 8) + 192;

        offset += 2 ;

        return true;
    }
    else
    {
        if(offset+5 > data_size)
            return false;

        data[offset++] = 0xff ;
        data[offset++] = (uint8_t)((S >> 24) & 255) ;
        data[offset++] = (uint8_t)((S >> 16) & 255) ;
        data[offset++] = (uint8_t)((S >>  8) & 255) ;
        data[offset++] = (uint8_t)((S      ) & 255) ;

        return true ;
    }
}

bool FileListIO::read125Size(const unsigned char *data,uint32_t data_size,uint32_t& offset,uint32_t& S)
{
    if(offset + 1 >= data_size) return false;

    uint8_t b1 = data[offset++] ;

    if(b1 < 192)
    {
        S = b1;
        return true ;
    }
    if(offset + 1 >= data_size) return false;

    uint8_t b2 = data[offset++] ;

    if(b1 < 224)
    {
        S = ((b1-192) << 8) + b2 + 192 ;
        return true;
    }

    if(b1 != 0xff)
        return false;

    if(offset + 3 >= data_size) return false;

    uint8_t b3 = data[offset++];
    uint8_t b4 = data[offset++];
    uint8_t b5 = data[offset++];

    S = (b2 << 24) | (b3 << 16) | (b4 << 8) | b5 ;
    return true;
}

bool FileListIO::saveEncryptedDataToFile(const std::string& fname,const unsigned char *data,uint32_t total_size)
{
    void *encryptedData = NULL ;
    int encDataLen = 0 ;

    if(!AuthSSL::getAuthSSL()->encrypt( encryptedData, encDataLen, data,total_size, AuthSSL::getAuthSSL()->OwnId()))
    {
        std::cerr << "Cannot encrypt encrypted file. Something's wrong." << std::endl;
        return false;
    }

    FILE *F = fopen( (fname+".tmp").c_str(),"wb" ) ;

    if(!F)
    {
        std::cerr << "Cannot open encrypted file for writing: " << fname+".tmp" << std::endl;

        free(encryptedData);
        return false;
    }
    if(fwrite(encryptedData,1,encDataLen,F) != (uint32_t)encDataLen)
    {
        std::cerr << "Could not write entire encrypted file. Out of disc space??" << std::endl;
        fclose(F) ;

        free(encryptedData);
        return false;
    }

    fclose(F) ;

    RsDirUtil::renameFile(fname+".tmp",fname) ;
#ifdef FIM_DEBUG
    std::cerr << "done." << std::endl ;
#endif

    free(encryptedData);
    return true;
}

bool FileListIO::loadEncryptedDataFromFile(const std::string& fname,unsigned char *& data,uint32_t& total_size)
{
   uint64_t file_size ;

    if(!RsDirUtil::checkFile( fname,file_size,true ) )
    {
#ifdef FIM_DEBUG
        std::cerr << "Encrypted file " << fname << " not available or empty." << std::endl;
#endif
        return false;
    }

    // read the binary stream into memory.
    //
    RsTemporaryMemory buffer(file_size) ;

    if(buffer == NULL)
       return false;

    FILE *F = fopen( fname.c_str(),"rb") ;
    if (!F)
    {
       std::cerr << "Cannot open encrypted file, filename " << fname << std::endl;
       return false;
    }
    if(fread(buffer,1,file_size,F) != file_size)
    {
       std::cerr << "Cannot read from file " + fname << ": something's wrong." << std::endl;
       fclose(F) ;
       return false;
    }
    fclose(F) ;

    // now decrypt
    void *decrypted_data =NULL;
    int decrypted_data_size =0;

    if(!AuthSSL::getAuthSSL()->decrypt(decrypted_data, decrypted_data_size, buffer, file_size))
    {
       std::cerr << "Cannot decrypt encrypted file. Something's wrong." << std::endl;
       return false;
    }

    data = (unsigned char*)decrypted_data ;
    total_size = decrypted_data_size ;

    return true;
}
