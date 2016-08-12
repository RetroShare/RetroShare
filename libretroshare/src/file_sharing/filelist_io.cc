#include "filelist_io.h"

bool FileListIO::writeField(      unsigned char*&buff,uint32_t& buff_size,uint32_t& offset,uint8_t       section_tag,const unsigned char *  val,uint32_t  size)
{
    if(!checkSectionSize(buff,buff_size,offset,size))
        return false;

    if(!writeSectionHeader(buff,buff_size,offset,FILE_LIST_IO_TAG_BINARY_DATA,size))
        return false;

    memcpy(&buff[offset],val,size) ;
    offset += size ;

    return true;
}

bool FileListIO::readField (const unsigned char *buff,uint32_t  buff_size,uint32_t& offset,uint8_t check_section_tag, unsigned char *& val,uint32_t& size)
{
    if(!readSectionHeader(buff,buff_size,offset,FILE_LIST_IO_TAG_BINARY_DATA,size))
        return false;

    val = (unsigned char *)rs_malloc(size) ;

    if(!val)
        return false;

    memcpy(val,&buff[offset],size);
    offset += size ;

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

        data[offset++] = (uint8_t)((S >> 8) + 192) ;
        data[offset++] = (uint8_t)((S & 255) - 192) ;

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

